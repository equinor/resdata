#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/well/well_segment.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_well_segment, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for WellSegment";

    m.def("_active", [](py::handle self) {
        return well_segment_active(from_cwrap<well_segment_type>(self));
    });
    m.def("_main_stem", [](py::handle self) {
        return well_segment_main_stem(from_cwrap<well_segment_type>(self));
    });
    m.def("_nearest_wellhead", [](py::handle self) {
        return well_segment_nearest_wellhead(
            from_cwrap<well_segment_type>(self));
    });
    m.def("_id", [](py::handle self) {
        return well_segment_get_id(from_cwrap<well_segment_type>(self));
    });
    m.def("_link_count", [](py::handle self) {
        return well_segment_get_link_count(from_cwrap<well_segment_type>(self));
    });
    m.def("_branch_id", [](py::handle self) {
        return well_segment_get_branch_id(from_cwrap<well_segment_type>(self));
    });
    m.def("_outlet_id", [](py::handle self) {
        return well_segment_get_outlet_id(from_cwrap<well_segment_type>(self));
    });
    m.def("_depth", [](py::handle self) {
        return well_segment_get_depth(from_cwrap<well_segment_type>(self));
    });
    m.def("_length", [](py::handle self) {
        return well_segment_get_length(from_cwrap<well_segment_type>(self));
    });
    m.def("_total_length", [](py::handle self) {
        return well_segment_get_total_length(
            from_cwrap<well_segment_type>(self));
    });
    m.def("_diameter", [](py::handle self) {
        return well_segment_get_diameter(from_cwrap<well_segment_type>(self));
    });
}
} // namespace
