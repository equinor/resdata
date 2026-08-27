#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ios>
#include <optional>
#include <string>
#include <system_error>

#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_grav.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_subsidence.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/layer.hpp>
#include <resdata/rd_sum_tstep.hpp>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>
#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_region.hpp>
#include <ert/geometry/geo_surface.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

template <typename T> T *cast_cwrap(py::handle obj) {
    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());
    return reinterpret_cast<T *>(pointer);
}

namespace {
int dup_fd(int fd) {
#ifdef _WIN32
    /* _dup() returns a non-inheritable descriptor. */
    return _dup(fd);
#else
    /* Duplicate with FD_CLOEXEC set, so that the descriptor does not leak into
       a child process if another thread forks and execs while we hold it. */
    return fcntl(fd, F_DUPFD_CLOEXEC, 0);
#endif
}

void close_fd(int fd) {
#ifdef _WIN32
    _close(fd);
#else
    ::close(fd);
#endif
}

void seek_fd(int fd, long pos) {
#ifdef _WIN32
    _lseek(fd, pos, SEEK_SET);
#else
    ::lseek(fd, pos, SEEK_SET);
#endif
}

/// Validates that @fd is open with access permissions compatible with @mode.
void check_fd_mode(int fd, const char *mode) {
#ifdef _WIN32
    if (_get_osfhandle(fd) == -1)
        throw py::value_error("Invalid file descriptor: " + std::to_string(fd));
#else
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        throw py::value_error("Invalid file descriptor: " + std::to_string(fd));

    int access_mode = flags & O_ACCMODE;
    bool wants_read = mode[0] == 'r' || std::strchr(mode, '+') != nullptr;
    bool wants_write =
        mode[0] == 'w' || mode[0] == 'a' || std::strchr(mode, '+') != nullptr;

    if (wants_read && access_mode != O_RDONLY && access_mode != O_RDWR)
        throw py::value_error("File descriptor " + std::to_string(fd) +
                              " is not open for reading, cannot open with "
                              "mode '" +
                              mode + "'");
    if (wants_write && access_mode != O_WRONLY && access_mode != O_RDWR)
        throw py::value_error("File descriptor " + std::to_string(fd) +
                              " is not open for writing, cannot open with "
                              "mode '" +
                              mode + "'");
#endif
}
} // namespace

FdStream::FdStream(int fd, const char *mode) : m_stream(nullptr), m_fd(-1) {
    check_fd_mode(fd, mode);

    m_fd = dup_fd(fd);
    if (m_fd == -1)
        throw py::value_error("Could not duplicate file descriptor " +
                              std::to_string(fd));

#ifdef _WIN32
    m_stream = _fdopen(m_fd, mode);
#else
    m_stream = fdopen(m_fd, mode);
#endif
    if (!m_stream) {
        close_fd(m_fd);
        m_fd = -1;
        throw py::value_error("Could not open file descriptor " +
                              std::to_string(fd) + " with mode '" + mode + "'");
    }
}

void FdStream::close() {
    if (!m_stream)
        return;

    FILE *stream = m_stream;
    m_stream = nullptr;

    /* Flush before querying the position: ftell() accounts for data which is
       still sitting in the stdio buffer, so seeking the descriptor to that
       position first would make the pending bytes be written at the wrong
       offset when the stream is closed. */
    errno = 0;
    bool failed = std::fflush(stream) != 0 || std::ferror(stream) != 0;
    int err = errno;

    /* stdio may have read ahead past the position that was actually consumed;
       rewind the (shared) file offset to the logical stream position so that
       the caller does not lose the buffered input. Streams which are not
       seekable, e.g. pipes, cannot be, and need not be, adjusted. */
    long pos = std::ftell(stream);
    if (pos >= 0)
        seek_fd(m_fd, pos);

    /* Closing flushes as well, and is where a delayed write error such as a
       full disk is most likely to surface. */
    errno = 0;
    if (std::fclose(stream) != 0) {
        failed = true;
        err = errno;
    }
    m_fd = -1;

    if (failed)
        throw std::ios_base::failure(
            "I/O error on file", std::error_code(err, std::generic_category()));
}

FdStream::~FdStream() {
    try {
        close();
    } catch (const std::ios_base::failure &) {
        /* Cannot report the error from a destructor; this only happens when
           close() was not called explicitly, i.e. when another exception is
           already on its way out. */
    }
}

py::object ResdataKW() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.resfile").attr("ResdataKW");
    }
    return cls;
}

template <> rd_kw_type *from_cwrap<rd_kw_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataKW()))
        throw py::type_error("Expected ResdataKW, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_kw_type>(obj);
}

template <> rd_kw_type *from_cwrap(std::optional<py::handle> obj) {
    if (!obj)
        return nullptr;

    if (!py::isinstance(*obj, ResdataKW()))
        throw py::type_error("Expected ResdataKW, got " +
                             static_cast<std::string>(py::repr(*obj)));
    return cast_cwrap<rd_kw_type>(*obj);
}

py::object Grid() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.grid").attr("Grid");
    }
    return cls;
}

template <> rd_grid_type *from_cwrap<rd_grid_type>(py::handle obj) {
    if (!py::isinstance(obj, Grid()))
        throw py::type_error("Expected Grid, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_grid_type>(obj);
}

py::object Summary() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.summary").attr("Summary");
    }
    return cls;
}

template <> rd_sum_type *from_cwrap<rd_sum_type>(py::handle obj) {
    if (!py::isinstance(obj, Summary()))
        throw py::type_error("Expected Summary, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_sum_type>(obj);
}

py::object SummaryKeyWordVector() {
    static py::object cls;
    if (!cls) {
        cls =
            py::module_::import("resdata.summary").attr("SummaryKeyWordVector");
    }
    return cls;
}

template <> rd_sum_vector_type *from_cwrap<rd_sum_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, SummaryKeyWordVector()))
        throw py::type_error("Expected SummaryKeyWordVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_sum_vector_type>(obj);
}

py::object ResDataType() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata").attr("ResDataType");
    }
    return cls;
}

template <>::rd_data_type *from_cwrap<::rd_data_type>(py::handle obj) {
    if (!py::isinstance(obj, ResDataType()))
        throw py::type_error("Expected ResDataType, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<::rd_data_type>(obj);
}

py::object ResdataSubsidence() {
    static py::object cls;
    if (!cls) {
        cls =
            py::module_::import("resdata.gravimetry").attr("ResdataSubsidence");
    }
    return cls;
}

template <> rd_subsidence_type *from_cwrap<rd_subsidence_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataSubsidence()))
        throw py::type_error("Expected ResdataSubsidence, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<rd_subsidence_type>(obj);
}

py::object ResdataRegion() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.grid").attr("ResdataRegion");
    }
    return cls;
}

template <> rd_region_type *from_cwrap<rd_region_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataRegion()))
        throw py::type_error("Expected ResdataRegion, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<rd_region_type>(obj);
}

py::object ResdataGrav() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.gravimetry").attr("ResdataGrav");
    }
    return cls;
}

template <> rd_grav_type *from_cwrap<rd_grav_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataGrav()))
        throw py::type_error("Expected ResdataGrav, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<rd_grav_type>(obj);
}

py::object CTime() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("CTime");
    }
    return cls;
}

py::object GeoPointset() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.geometry").attr("GeoPointset");
    }
    return cls;
}

template <> geo_pointset_type *from_cwrap<geo_pointset_type>(py::handle obj) {
    if (!py::isinstance(obj, GeoPointset()))
        throw py::type_error("Expected GeoPointset, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<geo_pointset_type>(obj);
}

py::object CPolylineCollection() {
    static py::object cls;
    if (!cls) {
        cls =
            py::module_::import("resdata.geometry").attr("CPolylineCollection");
    }
    return cls;
}

template <>
geo_polygon_collection_type *
from_cwrap<geo_polygon_collection_type>(py::handle obj) {
    if (!py::isinstance(obj, CPolylineCollection()))
        throw py::type_error("Expected CPolylineCollection, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<geo_polygon_collection_type>(obj);
}

py::object GeoRegion() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.geometry").attr("GeoRegion");
    }
    return cls;
}

template <> geo_region_type *from_cwrap<geo_region_type>(py::handle obj) {
    if (!py::isinstance(obj, GeoRegion()))
        throw py::type_error("Expected GeoRegion, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<geo_region_type>(obj);
}
py::object Surface() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.geometry").attr("Surface");
    }
    return cls;
}

template <> geo_surface_type *from_cwrap<geo_surface_type>(py::handle obj) {
    if (!py::isinstance(obj, Surface()))
        throw py::type_error("Expected Surface, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<geo_surface_type>(obj);
}

py::object FaultBlockLayer() {
    static py::object cls;
    if (!cls) {
        cls =
            py::module_::import("resdata.grid.faults").attr("FaultBlockLayer");
    }
    return cls;
}

template <>
fault_block_layer_type *from_cwrap<fault_block_layer_type>(py::handle obj) {
    if (!py::isinstance(obj, FaultBlockLayer()))
        throw py::type_error("Expected FaultBlockLayer, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<fault_block_layer_type>(obj);
}

py::object Layer() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.grid.faults").attr("Layer");
    }
    return cls;
}

template <> layer_type *from_cwrap<layer_type>(py::handle obj) {
    if (!py::isinstance(obj, Layer()))
        throw py::type_error("Expected Layer, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<layer_type>(obj);
}

py::object GeometryTools() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry").attr("GeometryTools");
    return cls;
}

py::object Polyline() {
    static py::object cls;
    if (!cls)
        cls = py::module_::import("resdata.geometry").attr("Polyline");
    return cls;
}

py::object SummaryVarType() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.summary.rd_sum_var_type")
                  .attr("SummaryVarType");
    }
    return cls;
}

py::object SummaryTStep() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.summary.rd_sum_tstep")
                  .attr("SummaryTStep");
    }
    return cls;
}

template <> rd_sum_tstep_type *from_cwrap<rd_sum_tstep_type>(py::handle obj) {
    if (!py::isinstance(obj, SummaryTStep()))
        throw py::type_error("Expected SummaryTStep, got " +
                             static_cast<std::string>(py::repr(obj)));

    return cast_cwrap<rd_sum_tstep_type>(obj);
}
