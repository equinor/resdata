#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/hash.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
void *void_ptr_from_python(py::handle obj) {
    if (obj.is_none())
        return nullptr;

    if (PyLong_Check(obj.ptr()))
        return PyLong_AsVoidPtr(obj.ptr());

    if (py::hasattr(obj, "value")) {
        py::object value = obj.attr("value");
        if (value.is_none())
            return nullptr;
        return PyLong_AsVoidPtr(value.ptr());
    }

    throw py::type_error("Expected c_void_p or integer pointer, got " +
                         static_cast<std::string>(py::repr(obj)));
}
} // namespace

PYBIND11_MODULE(_hash, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for Hash";

    m.def(
        "_alloc",
        []() { return reinterpret_cast<std::uintptr_t>(hash_alloc()); },
        py::return_value_policy::reference);

    m.def("_free", [](py::handle self) {
        hash_free(from_cwrap<hash_type>(self));
    });
    m.def("_size", [](py::handle self) {
        return hash_get_size(from_cwrap<hash_type>(self));
    });
    m.def("_keys", [](py::handle self) {
        return reinterpret_cast<std::uintptr_t>(
            hash_alloc_stringlist(from_cwrap<hash_type>(self)));
    }, py::return_value_policy::reference);
    m.def("_has_key", [](py::handle self, const char *key) {
        return hash_has_key(from_cwrap<hash_type>(self), key);
    });
    m.def("_get", [](py::handle self, const char *key) {
        return reinterpret_cast<std::uintptr_t>(
            hash_get(from_cwrap<hash_type>(self), key));
    }, py::return_value_policy::reference);
    m.def("_insert_ref", [](py::handle self, const char *key, py::handle value) {
        hash_insert_ref(from_cwrap<hash_type>(self), key,
                        void_ptr_from_python(value));
    });

    m.def("_get_string", [](py::handle self, const char *key) -> py::object {
        char *value = hash_get_string(from_cwrap<hash_type>(self), key);
        if (!value)
            return py::none();
        return py::str(value);
    });
    m.def("_insert_string", [](py::handle self, const char *key,
                               const char *value) {
        hash_insert_string(from_cwrap<hash_type>(self), key, value);
    });

    m.def("_get_int", [](py::handle self, const char *key) {
        return hash_get_int(from_cwrap<hash_type>(self), key);
    });
    m.def("_insert_int", [](py::handle self, const char *key, int value) {
        hash_insert_int(from_cwrap<hash_type>(self), key, value);
    });

    m.def("_get_double", [](py::handle self, const char *key) {
        return hash_get_double(from_cwrap<hash_type>(self), key);
    });
    m.def("_insert_double", [](py::handle self, const char *key, double value) {
        hash_insert_double(from_cwrap<hash_type>(self), key, value);
    });
}
