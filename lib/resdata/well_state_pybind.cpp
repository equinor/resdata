#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_state.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_well_state, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings between well_state.py and well_state.cpp";

    m.def("_name", [](py::handle self) {
        return std::string(
            well_state_get_name(from_cwrap<well_state_type>(self)));
    });
    m.def("_is_open", [](py::handle self) {
        return well_state_is_open(from_cwrap<well_state_type>(self));
    });
    m.def("_is_msw", [](py::handle self) {
        return well_state_is_MSW(from_cwrap<well_state_type>(self));
    });
    m.def("_well_number", [](py::handle self) {
        return well_state_get_well_nr(from_cwrap<well_state_type>(self));
    });
    m.def("_report_number", [](py::handle self) {
        return well_state_get_report_nr(from_cwrap<well_state_type>(self));
    });
    m.def("_has_segment_data", [](py::handle self) {
        return well_state_has_segment_data(from_cwrap<well_state_type>(self));
    });
    m.def("_sim_time", [](py::handle self) {
        return static_cast<std::int64_t>(
            well_state_get_sim_time(from_cwrap<well_state_type>(self)));
    });
    m.def("_well_type", [](py::handle self) {
        return static_cast<int>(
            well_state_get_type(from_cwrap<well_state_type>(self)));
    });
    m.def("_has_global_connections", [](py::handle self) {
        return well_state_has_global_connections(
            from_cwrap<well_state_type>(self));
    });

    m.def("_oil_rate", [](py::handle self) {
        return well_state_get_oil_rate(from_cwrap<well_state_type>(self));
    });
    m.def("_gas_rate", [](py::handle self) {
        return well_state_get_gas_rate(from_cwrap<well_state_type>(self));
    });
    m.def("_water_rate", [](py::handle self) {
        return well_state_get_water_rate(from_cwrap<well_state_type>(self));
    });
    m.def("_volume_rate", [](py::handle self) {
        return well_state_get_volume_rate(from_cwrap<well_state_type>(self));
    });
    m.def("_oil_rate_si", [](py::handle self) {
        return well_state_get_oil_rate_si(from_cwrap<well_state_type>(self));
    });
    m.def("_gas_rate_si", [](py::handle self) {
        return well_state_get_gas_rate_si(from_cwrap<well_state_type>(self));
    });
    m.def("_water_rate_si", [](py::handle self) {
        return well_state_get_water_rate_si(from_cwrap<well_state_type>(self));
    });
    m.def("_volume_rate_si", [](py::handle self) {
        return well_state_get_volume_rate_si(from_cwrap<well_state_type>(self));
    });

    m.def("_well_head", [](py::handle self) -> py::object {
        const well_conn_type *wellhead =
            well_state_get_global_wellhead(from_cwrap<well_state_type>(self));
        if (!wellhead)
            return py::none();
        return WellConnection().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(wellhead), self);
    });

    m.def("_num_segments", [](py::handle self) {
        return well_segment_collection_get_size(
            well_state_get_segments(from_cwrap<well_state_type>(self)));
    });

    m.def("_segments", [](py::handle self) {
        auto *segments =
            well_state_get_segments(from_cwrap<well_state_type>(self));
        int size = well_segment_collection_get_size(segments);
        std::vector<py::object> result;
        result.reserve(size);
        for (int i = 0; i < size; i++) {
            well_segment_type *segment =
                well_segment_collection_iget(segments, i);
            result.push_back(WellSegment().attr("createCReference")(
                reinterpret_cast<std::uintptr_t>(segment), self));
        }
        return result;
    });

    m.def("_iget_segment", [](py::handle self, size_t index) {
        auto *segments =
            well_state_get_segments(from_cwrap<well_state_type>(self));
        return WellSegment().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(
                well_segment_collection_iget(segments, index)),
            self);
    });

    m.def("global_connections", [](py::handle self) {
        auto well_conns = well_state_get_global_connections(
            from_cwrap<well_state_type>(self));
        std::vector<py::object> result;
        if (!well_conns)
            return result;
        std::transform(well_conns->begin(), well_conns->end(),
                       std::back_inserter(result), [self](auto &p) {
                           return WellConnection().attr("createCReference")(
                               reinterpret_cast<std::uintptr_t>(p.get()), self);
                       });
        return result;
    });
}
} // namespace
