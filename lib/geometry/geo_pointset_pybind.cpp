#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_pointset.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_geo_pointset, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for GeoPointset";

    m.def(
        "_alloc",
        [](bool external_z) {
            return reinterpret_cast<std::uintptr_t>(
                geo_pointset_alloc(external_z));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        geo_pointset_free(from_cwrap<geo_pointset_type>(self));
    });
    m.def("_get_size", [](py::handle self) {
        return geo_pointset_get_size(from_cwrap<geo_pointset_type>(self));
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return geo_pointset_equal(from_cwrap<geo_pointset_type>(self),
                                  from_cwrap<geo_pointset_type>(other));
    });
    m.def("_iget_z", [](py::handle self, int index) {
        return geo_pointset_iget_z(from_cwrap<geo_pointset_type>(self), index);
    });
}
} // namespace
