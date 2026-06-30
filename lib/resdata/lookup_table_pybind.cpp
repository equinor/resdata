#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/lookup_table.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_lookup_table, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for LookupTable";

    m.def(
        "_alloc",
        []() {
            return reinterpret_cast<std::uintptr_t>(lookup_table_alloc_empty());
        },
        py::return_value_policy::reference);

    m.def("_max", [](py::handle self) {
        return lookup_table_get_max_value(from_cwrap<lookup_table_type>(self));
    });
    m.def("_min", [](py::handle self) {
        return lookup_table_get_min_value(from_cwrap<lookup_table_type>(self));
    });
    m.def("_arg_max", [](py::handle self) {
        return lookup_table_get_max_arg(from_cwrap<lookup_table_type>(self));
    });
    m.def("_arg_min", [](py::handle self) {
        return lookup_table_get_min_arg(from_cwrap<lookup_table_type>(self));
    });
    m.def("_append", [](py::handle self, double x, double y) {
        lookup_table_append(from_cwrap<lookup_table_type>(self), x, y);
    });
    m.def("_size", [](py::handle self) {
        return lookup_table_get_size(from_cwrap<lookup_table_type>(self));
    });
    m.def("_interp", [](py::handle self, double x) {
        return lookup_table_interp(from_cwrap<lookup_table_type>(self), x);
    });
    m.def("_free", [](py::handle self) {
        lookup_table_free(from_cwrap<lookup_table_type>(self));
    });
    m.def("_set_low_limit", [](py::handle self, double limit) {
        lookup_table_set_low_limit(from_cwrap<lookup_table_type>(self), limit);
    });
    m.def("_set_high_limit", [](py::handle self, double limit) {
        lookup_table_set_high_limit(from_cwrap<lookup_table_type>(self), limit);
    });
    m.def("_has_low_limit", [](py::handle self) {
        return lookup_table_has_low_limit(from_cwrap<lookup_table_type>(self));
    });
    m.def("_has_high_limit", [](py::handle self) {
        return lookup_table_has_high_limit(from_cwrap<lookup_table_type>(self));
    });
}
