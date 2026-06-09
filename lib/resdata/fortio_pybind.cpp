#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/FortIO.hpp>

namespace py = pybind11;

namespace {
template <typename T> T *from_cwrap(py::handle obj) {
    if (obj.is_none())
        return nullptr;

    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());

    return reinterpret_cast<T *>(pointer);
}

PYBIND11_MODULE(_fortio, m) {
    m.doc() = "pybind11 bindings between fortio.py and FortIO.cpp";

    m.def(
        "_open_reader",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(
                fortio_open_reader(filename.c_str(), fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def(
        "_open_writer",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(
                fortio_open_writer(filename.c_str(), fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def(
        "_open_readwrite",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(
                fortio_open_readwrite(filename.c_str(), fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def(
        "_open_append",
        [](const std::string &filename, bool fmt_file, bool endian_flip) {
            return reinterpret_cast<std::uintptr_t>(
                fortio_open_append(filename.c_str(), fmt_file, endian_flip));
        },
        py::return_value_policy::reference);

    m.def("_guess_fortran", [](const std::string &filename, bool endian_flip) {
        return fortio_looks_like_fortran_file(filename.c_str(), endian_flip);
    });

    m.def("_get_position", [](py::handle self) {
        return fortio_ftell(from_cwrap<fortio_type>(self));
    });

    m.def("_seek", [](py::handle self, offset_type pos, int whence) {
        return fortio_fseek(from_cwrap<fortio_type>(self), pos, whence);
    });

    m.def("_close", [](py::handle self) {
        return fortio_fclose(from_cwrap<fortio_type>(self));
    });

    m.def("_truncate", [](py::handle self, offset_type size) {
        return fortio_ftruncate(from_cwrap<fortio_type>(self), size);
    });

    m.def("_filename", [](py::handle self) {
        const char *name = fortio_filename_ref(from_cwrap<fortio_type>(self));
        if (name == nullptr)
            return std::string();
        return std::string(name);
    });
}
} // namespace
