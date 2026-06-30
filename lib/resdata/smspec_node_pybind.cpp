#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/smspec_node.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

using smspec_node_type = rd::smspec_node;

namespace {
py::object nullable_string(const char *value) {
    if (!value)
        return py::none();
    return py::str(value);
}
} // namespace

PYBIND11_MODULE(_rd_smspec_node, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResdataSMSPECNode";

    m.def("_node_is_total", [](py::handle self) {
        return smspec_node_is_total(from_cwrap<smspec_node_type>(self));
    });
    m.def("_node_is_historical", [](py::handle self) {
        return smspec_node_is_historical(from_cwrap<smspec_node_type>(self));
    });
    m.def("_node_is_rate", [](py::handle self) {
        return smspec_node_is_rate(from_cwrap<smspec_node_type>(self));
    });
    m.def("_node_unit", [](py::handle self) {
        return nullable_string(
            smspec_node_get_unit(from_cwrap<smspec_node_type>(self)));
    });
    m.def("_node_wgname", [](py::handle self) {
        return nullable_string(
            smspec_node_get_wgname(from_cwrap<smspec_node_type>(self)));
    });
    m.def("_node_keyword", [](py::handle self) {
        return nullable_string(
            smspec_node_get_keyword(from_cwrap<smspec_node_type>(self)));
    });
    m.def("_node_num", [](py::handle self) {
        return smspec_node_get_num(from_cwrap<smspec_node_type>(self));
    });
    m.def("_node_need_num", [](py::handle self) {
        return smspec_node_need_nums(from_cwrap<smspec_node_type>(self));
    });
    m.def("_gen_key1", [](py::handle self) {
        return nullable_string(
            smspec_node_get_gen_key1(from_cwrap<smspec_node_type>(self)));
    });
    m.def("_gen_key2", [](py::handle self) {
        return nullable_string(
            smspec_node_get_gen_key2(from_cwrap<smspec_node_type>(self)));
    });
    m.def("_var_type", [](py::handle self) {
        return static_cast<int>(
            smspec_node_get_var_type(from_cwrap<smspec_node_type>(self)));
    });
    m.def("_cmp", [](py::handle self, py::handle other) {
        return smspec_node_cmp(from_cwrap<smspec_node_type>(self),
                               from_cwrap<smspec_node_type>(other));
    });
    m.def("_get_default", [](py::handle self) {
        return smspec_node_get_default(from_cwrap<smspec_node_type>(self));
    });
}
