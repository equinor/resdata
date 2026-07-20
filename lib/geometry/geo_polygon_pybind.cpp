#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_polygon.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_cpolyline, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for CPolyline";

    m.def(
        "_alloc_new",
        [](std::optional<std::string> name) {
            return reinterpret_cast<std::uintptr_t>(
                geo_polygon_alloc(name ? name->c_str() : nullptr));
        },
        py::return_value_policy::reference);
    m.def(
        "_fread_alloc_irap",
        [](std::string filename) {
            return reinterpret_cast<std::uintptr_t>(
                geo_polygon_fload_alloc_irap(filename.c_str()));
        },
        py::return_value_policy::reference);
    m.def("_add_point", [](py::handle self, double x, double y) {
        geo_polygon_add_point(from_cwrap<geo_polygon_type>(self), x, y);
    });
    m.def("_add_point_front", [](py::handle self, double x, double y) {
        geo_polygon_add_point_front(from_cwrap<geo_polygon_type>(self), x, y);
    });
    m.def("_free", [](py::handle self) {
        geo_polygon_free(from_cwrap<geo_polygon_type>(self));
    });
    m.def("_size", [](py::handle self) {
        return geo_polygon_get_size(from_cwrap<geo_polygon_type>(self));
    });
    m.def("_iget_xy", [](py::handle self, int index) {
        double x = 0;
        double y = 0;
        geo_polygon_iget_xy(from_cwrap<geo_polygon_type>(self), index, &x, &y);
        return std::make_tuple(x, y);
    });
    m.def("_segment_intersects",
          [](py::handle self, double x1, double y1, double x2, double y2) {
              return geo_polygon_segment_intersects(
                  from_cwrap<geo_polygon_type>(self), x1, y1, x2, y2);
          });
    m.def("_get_name", [](py::handle self) -> py::object {
        const char *name =
            geo_polygon_get_name(from_cwrap<geo_polygon_type>(self));
        if (!name)
            return py::none();
        return py::str(name);
    });
    m.def("_set_name", [](py::handle self, std::optional<std::string> name) {
        geo_polygon_set_name(from_cwrap<geo_polygon_type>(self),
                             name ? name->c_str() : nullptr);
    });
    m.def("_segment_length", [](py::handle self) {
        return geo_polygon_get_length(from_cwrap<geo_polygon_type>(self));
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return geo_polygon_equal(from_cwrap<geo_polygon_type>(self),
                                 from_cwrap<geo_polygon_type>(other));
    });
}
} // namespace
