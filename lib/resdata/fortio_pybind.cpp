#include <cstdint>

#include <string>
#include <ios>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <resdata/FortIO.hpp>

#include "ert/util/util.hpp"

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_fortio, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings between fortio.py and FortIO.cpp";

    m.def(
        "_open_reader",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(new ERT::FortIO(
                filename, std::ios_base::in, fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def(
        "_open_writer",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(new ERT::FortIO(
                filename, std::ios_base::out, fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def(
        "_open_readwrite",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(new ERT::FortIO(
                filename, std::ios_base::in | std::ios_base::out, fmt_file,
                endian_flip));
        },
        py::return_value_policy::reference);

    m.def(
        "_open_append",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(new ERT::FortIO(
                filename, std::ios_base::app, fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def("_guess_fortran", [](const std::string &filename, bool endian_flip) {
        return ERT::FortIO::looks_like_fortran_file(filename.c_str(),
                                                    endian_flip);
    });

    m.def("_get_position", [](py::handle self) {
        return from_cwrap<fortio_type>(self)->ftell();
    });

    m.def("_seek", [](py::handle self, offset_type pos, int whence) {
        return from_cwrap<fortio_type>(self)->fseek(pos, whence);
    });

    m.def("_close",
          [](py::handle self) { delete from_cwrap<fortio_type>(self); });

    m.def("_truncate", [](py::handle self, offset_type size) {
        return from_cwrap<fortio_type>(self)->ftruncate(size);
    });

    m.def("_filename", [](py::handle self) {
        const char *name = from_cwrap<fortio_type>(self)->filename_ref();
        if (name == nullptr)
            return std::string();
        return std::string(name);
    });
}
} // namespace
