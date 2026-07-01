#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/well/well_conn.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_well_connection, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for WellConnection";

    m.def("_i", [](py::handle self) {
        return well_conn_get_i(from_cwrap<well_conn_type>(self));
    });
    m.def("_j", [](py::handle self) {
        return well_conn_get_j(from_cwrap<well_conn_type>(self));
    });
    m.def("_k", [](py::handle self) {
        return well_conn_get_k(from_cwrap<well_conn_type>(self));
    });
    m.def("_segment_id", [](py::handle self) {
        return well_conn_get_segment_id(from_cwrap<well_conn_type>(self));
    });
    m.def("_is_open", [](py::handle self) {
        return well_conn_open(from_cwrap<well_conn_type>(self));
    });
    m.def("_is_msw", [](py::handle self) {
        return well_conn_MSW(from_cwrap<well_conn_type>(self));
    });
    m.def("_fracture_connection", [](py::handle self) {
        return well_conn_fracture_connection(from_cwrap<well_conn_type>(self));
    });
    m.def("_matrix_connection", [](py::handle self) {
        return well_conn_matrix_connection(from_cwrap<well_conn_type>(self));
    });
    m.def("_connection_factor", [](py::handle self) {
        return well_conn_get_connection_factor(
            from_cwrap<well_conn_type>(self));
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return well_conn_equal(from_cwrap<well_conn_type>(self),
                               from_cwrap<well_conn_type>(other));
    });
    m.def("_get_dir", [](py::handle self) {
        return static_cast<int>(
            well_conn_get_dir(from_cwrap<well_conn_type>(self)));
    });
    m.def("_oil_rate", [](py::handle self) {
        return well_conn_get_oil_rate(from_cwrap<well_conn_type>(self));
    });
    m.def("_gas_rate", [](py::handle self) {
        return well_conn_get_gas_rate(from_cwrap<well_conn_type>(self));
    });
    m.def("_water_rate", [](py::handle self) {
        return well_conn_get_water_rate(from_cwrap<well_conn_type>(self));
    });
    m.def("_volume_rate", [](py::handle self) {
        return well_conn_get_volume_rate(from_cwrap<well_conn_type>(self));
    });
    m.def("_oil_rate_si", [](py::handle self) {
        return well_conn_get_oil_rate_si(from_cwrap<well_conn_type>(self));
    });
    m.def("_gas_rate_si", [](py::handle self) {
        return well_conn_get_gas_rate_si(from_cwrap<well_conn_type>(self));
    });
    m.def("_water_rate_si", [](py::handle self) {
        return well_conn_get_water_rate_si(from_cwrap<well_conn_type>(self));
    });
    m.def("_volume_rate_si", [](py::handle self) {
        return well_conn_get_volume_rate_si(from_cwrap<well_conn_type>(self));
    });
}
} // namespace
