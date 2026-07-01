#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/well/well_ts.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_well_time_line, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for WellTimeLine";

    m.def("_size", [](py::handle self) {
        return well_ts_get_size(from_cwrap<well_ts_type>(self));
    });
    m.def("_name", [](py::handle self) {
        return std::string(well_ts_get_name(from_cwrap<well_ts_type>(self)));
    });
    m.def("_iget", [](py::handle self, int index) {
        well_state_type *well_state =
            well_ts_iget_state(from_cwrap<well_ts_type>(self), index);
        return WellState().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(well_state), self);
    });
}
} // namespace
