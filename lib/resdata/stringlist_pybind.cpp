#include <cstdint>
#include <cstdlib>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/stringlist.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
py::object nullable_string(const char *value) {
    if (!value)
        return py::none();
    return py::str(std::string(value));
}

py::object owned_nullable_string(char *value) {
    if (!value)
        return py::none();

    std::string result(value);
    free(value);
    return py::str(result);
}
} // namespace

PYBIND11_MODULE(_stringlist, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for StringList";

    m.def(
        "_alloc",
        []() {
            return reinterpret_cast<std::uintptr_t>(stringlist_alloc_new());
        },
        py::return_value_policy::reference);

    m.def("_free", [](py::handle self) {
        stringlist_free(from_cwrap<stringlist_type>(self));
    });
    m.def("_append", [](py::handle self, const std::string &value) {
        stringlist_append_copy(from_cwrap<stringlist_type>(self),
                               value.c_str());
    });
    m.def("_iget", [](py::handle self, int index) {
        return nullable_string(
            stringlist_iget(from_cwrap<stringlist_type>(self), index));
    });
    m.def("_front", [](py::handle self) {
        return nullable_string(
            stringlist_front(from_cwrap<stringlist_type>(self)));
    });
    m.def("_back", [](py::handle self) {
        return nullable_string(
            stringlist_back(from_cwrap<stringlist_type>(self)));
    });
    m.def("_iget_copy", [](py::handle self, int index) {
        return owned_nullable_string(
            stringlist_iget_copy(from_cwrap<stringlist_type>(self), index));
    });
    m.def("_iset", [](py::handle self, int index, const std::string &value) {
        stringlist_iset_copy(from_cwrap<stringlist_type>(self), index,
                             value.c_str());
    });
    m.def("_get_size", [](py::handle self) {
        return stringlist_get_size(from_cwrap<stringlist_type>(self));
    });
    m.def("_contains", [](py::handle self, const std::string &value) {
        return stringlist_contains(from_cwrap<stringlist_type>(self),
                                   value.c_str());
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return stringlist_equal(from_cwrap<stringlist_type>(self),
                                from_cwrap<stringlist_type>(other));
    });
    m.def("_sort", [](py::handle self, int cmp_flag) {
        stringlist_python_sort(from_cwrap<stringlist_type>(self), cmp_flag);
    });
    m.def("_pop", [](py::handle self) {
        return owned_nullable_string(
            stringlist_pop(from_cwrap<stringlist_type>(self)));
    });
    m.def("_last", [](py::handle self) {
        return nullable_string(
            stringlist_get_last(from_cwrap<stringlist_type>(self)));
    });
    m.def("_find_first", [](py::handle self, const std::string &value) {
        return stringlist_find_first(from_cwrap<stringlist_type>(self),
                                     value.c_str());
    });
}
