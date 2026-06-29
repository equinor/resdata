#include <cstdint>

#include <algorithm>
#include <iterator>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/well/well_state.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_well_state, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings between well_state.py and well_state.cpp";

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
