#include <cstdio>
#include <optional>
#include <string>

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

#include <ert/util/double_vector.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/int_vector.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

template <typename T> T *cast_cwrap(py::handle obj) {
    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());
    return reinterpret_cast<T *>(pointer);
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

py::object StringList() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("StringList");
    }
    return cls;
}

template <> stringlist_type *from_cwrap<stringlist_type>(py::handle obj) {
    if (!py::isinstance(obj, StringList()))
        throw py::type_error("Expected StringList, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<stringlist_type>(obj);
}

py::object TimeVector() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("TimeVector");
    }
    return cls;
}

template <> time_t_vector_type *from_cwrap<time_t_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, TimeVector()))
        throw py::type_error("Expected TimeVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<time_t_vector_type>(obj);
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

py::object DoubleVector() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("DoubleVector");
    }
    return cls;
}

template <> double_vector_type *from_cwrap<double_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, DoubleVector()))
        throw py::type_error("Expected DoubleVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<double_vector_type>(obj);
}

py::object CFILE() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("cwrap.cfile").attr("CWrapFile");
    }
    return cls;
}

template <> FILE *from_cwrap<FILE>(py::handle obj) {
    if (!py::isinstance(obj, CFILE()))
        throw py::type_error("Expected CFILE, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<FILE>(obj);
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

py::object IntVector() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.util.util").attr("IntVector");
    }
    return cls;
}

template <> int_vector_type *from_cwrap<int_vector_type>(py::handle obj) {
    if (!py::isinstance(obj, IntVector()))
        throw py::type_error("Expected IntVector, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<int_vector_type>(obj);
}

py::object ResdataFile() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.resfile").attr("ResdataFile");
    }
    return cls;
}

template <> rd_file_type *from_cwrap<rd_file_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataFile()))
        throw py::type_error("Expected ResdataFile, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_file_type>(obj);
}

py::object ResdataFileView() {
    static py::object cls;
    if (!cls) {
        cls = py::module_::import("resdata.resfile").attr("ResdataFileView");
    }
    return cls;
}

template <> rd_file_view_type *from_cwrap<rd_file_view_type>(py::handle obj) {
    if (!py::isinstance(obj, ResdataFileView()))
        throw py::type_error("Expected ResdataFileView, got " +
                             static_cast<std::string>(py::repr(obj)));
    return cast_cwrap<rd_file_view_type>(obj);
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
