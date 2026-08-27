#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_polygon_collection.hpp>
#include "ert/geometry/geo_polygon.hpp"
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_cpolyline_collection, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for CPolylineCollection";

    m.def(
        "_alloc_new",
        []() {
            return reinterpret_cast<std::uintptr_t>(
                geo_polygon_collection_alloc());
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        geo_polygon_collection_free(
            from_cwrap<geo_polygon_collection_type>(self));
    });
    m.def("_size", [](py::handle self) {
        return geo_polygon_collection_size(
            from_cwrap<geo_polygon_collection_type>(self));
    });
    m.def("_create_polyline",
          [](py::handle self, std::optional<std::string> name) {
              return geo_polygon_collection_create_polygon(
                  from_cwrap<geo_polygon_collection_type>(self),
                  name ? name->c_str() : nullptr);
          });
    m.def("_has_polyline",
          [](py::handle self, std::optional<std::string> name) {
              return geo_polygon_collection_has_polygon(
                  from_cwrap<geo_polygon_collection_type>(self),
                  name ? name->c_str() : nullptr);
          });
    m.def("_iget", [](py::handle self, size_t index) {
        return geo_polygon_collection_iget_polygon(
            from_cwrap<geo_polygon_collection_type>(self), index);
    });
    m.def("_get", [](py::handle self, const std::string &name) {
        return geo_polygon_collection_get_polygon(
            from_cwrap<geo_polygon_collection_type>(self), name);
    });
    m.def("_add_polyline",
          [](py::handle self, std::shared_ptr<rd::Polygon> polyline) {
              geo_polygon_collection_add_polygon(
                  from_cwrap<geo_polygon_collection_type>(self), polyline);
          });
}
} // namespace
