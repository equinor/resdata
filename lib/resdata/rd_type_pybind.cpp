#include <pybind11/pybind11.h>

#include <resdata/rd_type.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

#include <cstdint>

#include <string>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_type, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings between rd_type.py and rd_type.cpp";

    m.def("_name", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_name(*rd_type);
    });

    m.def("_create_from_name", [](const std::string &name) {
        return reinterpret_cast<std::uintptr_t>(
            new rd_data_type(rd_type_create_from_name(name.c_str())));
    });

    m.def("_create_from_type", [](int type_enum) {
        return reinterpret_cast<std::uintptr_t>(new rd_data_type(
            rd_type_create_from_type(static_cast<rd_type_enum>(type_enum))));
    });

    m.def("_create", [](int type_enum, size_t element_size) {
        return reinterpret_cast<std::uintptr_t>(new rd_data_type(rd_type_create(
            static_cast<rd_type_enum>(type_enum), element_size)));
    });

    m.def("_get_type", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return static_cast<int>(rd_type_get_type(*rd_type));
    });

    m.def("_get_sizeof_iotype", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_get_sizeof_iotype(*rd_type);
    });

    m.def("_is_int", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_int(*rd_type);
    });

    m.def("_is_char", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_char(*rd_type);
    });

    m.def("_is_float", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_float(*rd_type);
    });

    m.def("_is_double", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_double(*rd_type);
    });

    m.def("_is_mess", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_mess(*rd_type);
    });

    m.def("_is_bool", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_bool(*rd_type);
    });

    m.def("_is_string", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_string(*rd_type);
    });

    m.def("_is_numeric", [](py::handle data_type) {
        auto *rd_type = from_cwrap<rd_data_type>(data_type);
        return rd_type_is_numeric(*rd_type);
    });

    m.def("_is_equal", [](py::handle data_type1, py::handle data_type2) {
        auto *rd_type1 = from_cwrap<rd_data_type>(data_type1);
        auto *rd_type2 = from_cwrap<rd_data_type>(data_type2);
        return rd_type_is_equal(*rd_type1, *rd_type2);
    });

    m.def("_alloc_copy", [](py::handle src_type) {
        auto *rd_type = from_cwrap<rd_data_type>(src_type);
        return reinterpret_cast<std::uintptr_t>(new rd_data_type(*rd_type));
    });

    m.def("_free", [](py::handle obj) {
        auto *rd_type = from_cwrap<rd_data_type>(obj);
        delete rd_type;
    });
}
} // namespace
