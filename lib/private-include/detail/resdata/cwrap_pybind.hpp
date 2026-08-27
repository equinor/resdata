#pragma once
#include <cstdio>
#include <optional>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

template <typename T> T *from_cwrap(pybind11::handle obj);
template <typename T> T *from_cwrap(std::optional<pybind11::handle> obj);

/// RAII wrapper around a stdio stream opened on a Python file descriptor.
///
/// The descriptor is owned by the Python file object and must stay open, so
/// the stream is opened on a *duplicate* of it. The duplicate shares the file
/// offset with the original descriptor, which means the position seen by
/// Python follows along, while closing the stream releases the FILE object and
/// its buffer without closing the caller's file.
///
/// Closing also sets the shared file offset to the logical position of the
/// stream, so that any read-ahead performed by stdio is not observed as lost
/// input on the Python side.
///
/// Call close() explicitly when done in order to have write errors reported;
/// the destructor is only a fallback for the case where the stream is
/// abandoned because an exception is propagating.
class FdStream {
public:
    /// Checks that the file descriptor @fd was opened with access permissions
    /// compatible with @mode (as would be passed to fdopen(3)), and then opens
    /// a stream on a duplicate of it. Throws a pybind11::value_error if the
    /// descriptor is invalid or does not support the requested mode.
    FdStream(int fd, const char *mode);
    ~FdStream();

    FdStream(const FdStream &) = delete;
    FdStream &operator=(const FdStream &) = delete;

    /// Flushes and closes the stream. Throws std::ios_base::failure, which is
    /// translated to an OSError, if an error occurred on the stream; note that
    /// errors such as a full disk are only detected once the stdio buffer is
    /// written out, i.e. typically here rather than at the point where the
    /// data was formatted.
    void close();

    operator FILE *() const { return m_stream; }

private:
    FILE *m_stream;
    /// The duplicated descriptor, owned by m_stream.
    int m_fd;
};

/// Sets up custom exception translators
inline void register_exceptions(pybind11::module &m) {
    pybind11::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p)
                std::rethrow_exception(p);
        } catch (const std::ios_base::failure &e) {
            PyErr_SetString(PyExc_OSError, e.what());
        }
    });
}

pybind11::object CTime();
pybind11::object ResdataKW();
pybind11::object GeoPointset();
pybind11::object FaultBlockLayer();
pybind11::object Layer();
pybind11::object GeometryTools();
pybind11::object Polyline();
pybind11::object CPolylineCollection();
pybind11::object SummaryVarType();
