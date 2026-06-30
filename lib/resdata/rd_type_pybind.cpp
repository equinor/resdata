#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_type.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

extern "C" {
rd_data_type *rd_type_alloc_python(rd_type_enum type, size_t element_size);
rd_data_type *rd_type_alloc_from_type_python(rd_type_enum type);
rd_data_type *rd_type_alloc_from_name_python(const char *name);
void rd_type_free_python(rd_data_type *data_type);
rd_type_enum rd_type_get_type_python(const rd_data_type *rd_type);
const char *rd_type_alloc_name_python(const rd_data_type *rd_type);
size_t rd_type_get_sizeof_iotype_python(const rd_data_type *rd_type);
bool rd_type_is_numeric_python(const rd_data_type *rd_type);
bool rd_type_is_equal_python(const rd_data_type *rd_type1,
                             const rd_data_type *rd_type2);
bool rd_type_is_char_python(const rd_data_type *rd_type);
bool rd_type_is_int_python(const rd_data_type *rd_type);
bool rd_type_is_float_python(const rd_data_type *rd_type);
bool rd_type_is_double_python(const rd_data_type *rd_type);
bool rd_type_is_mess_python(const rd_data_type *rd_type);
bool rd_type_is_bool_python(const rd_data_type *rd_type);
bool rd_type_is_string_python(const rd_data_type *rd_type);
}

PYBIND11_MODULE(_rd_type, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResDataType";

    m.def(
        "_alloc",
        [](int type, size_t element_size) {
            return reinterpret_cast<std::uintptr_t>(rd_type_alloc_python(
                static_cast<rd_type_enum>(type), element_size));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_from_type",
        [](int type) {
            return reinterpret_cast<std::uintptr_t>(
                rd_type_alloc_from_type_python(static_cast<rd_type_enum>(type)));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_from_name",
        [](std::string name) {
            return reinterpret_cast<std::uintptr_t>(
                rd_type_alloc_from_name_python(name.c_str()));
        },
        py::return_value_policy::reference);

    m.def("_free", [](py::handle self) {
        rd_type_free_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_get_type", [](py::handle self) {
        return static_cast<int>(
            rd_type_get_type_python(from_cwrap<::rd_data_type>(self)));
    });
    m.def("_get_element_size", [](py::handle self) {
        return rd_type_get_sizeof_iotype_python(
            from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_int", [](py::handle self) {
        return rd_type_is_int_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_char", [](py::handle self) {
        return rd_type_is_char_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_float", [](py::handle self) {
        return rd_type_is_float_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_double", [](py::handle self) {
        return rd_type_is_double_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_mess", [](py::handle self) {
        return rd_type_is_mess_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_bool", [](py::handle self) {
        return rd_type_is_bool_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_string", [](py::handle self) {
        return rd_type_is_string_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_get_name", [](py::handle self) {
        const char *name =
            rd_type_alloc_name_python(from_cwrap<::rd_data_type>(self));
        std::string result(name);
        free(const_cast<char *>(name));
        return result;
    });
    m.def("_is_numeric", [](py::handle self) {
        return rd_type_is_numeric_python(from_cwrap<::rd_data_type>(self));
    });
    m.def("_is_equal", [](py::handle self, py::handle other) {
        return rd_type_is_equal_python(from_cwrap<::rd_data_type>(self),
                                       from_cwrap<::rd_data_type>(other));
    });
}
