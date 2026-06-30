#include <cstdint>
#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_region.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
const double *line_vector_data(const std::vector<double> &values,
                               const char *name) {
    if (values.size() != 2)
        throw py::value_error(std::string(name) + " must contain exactly 2 values");
    return values.data();
}
} // namespace

namespace {
PYBIND11_MODULE(_geo_region, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for GeoRegion";

    m.def(
        "_alloc",
        [](py::handle pointset, bool preselect) {
            return reinterpret_cast<std::uintptr_t>(
                geo_region_alloc(from_cwrap<geo_pointset_type>(pointset), preselect));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        geo_region_free(from_cwrap<geo_region_type>(self));
    });
    m.def("_reset", [](py::handle self) {
        geo_region_reset(from_cwrap<geo_region_type>(self));
    });
    m.def(
        "_get_index_list",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                geo_region_get_index_list(from_cwrap<geo_region_type>(self)));
        },
        py::return_value_policy::reference);

    m.def("_select_inside_polygon", [](py::handle self, py::handle polygon) {
        geo_region_select_inside_polygon(from_cwrap<geo_region_type>(self),
                                         from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_select_outside_polygon", [](py::handle self, py::handle polygon) {
        geo_region_select_outside_polygon(from_cwrap<geo_region_type>(self),
                                          from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_deselect_inside_polygon", [](py::handle self, py::handle polygon) {
        geo_region_deselect_inside_polygon(from_cwrap<geo_region_type>(self),
                                           from_cwrap<geo_polygon_type>(polygon));
    });
    m.def("_deselect_outside_polygon", [](py::handle self, py::handle polygon) {
        geo_region_deselect_outside_polygon(from_cwrap<geo_region_type>(self),
                                            from_cwrap<geo_polygon_type>(polygon));
    });

    m.def("_select_above_line", [](py::handle self, std::vector<double> xcoords,
                                    std::vector<double> ycoords) {
        geo_region_select_above_line(from_cwrap<geo_region_type>(self),
                                     line_vector_data(xcoords, "xcoords"),
                                     line_vector_data(ycoords, "ycoords"));
    });
    m.def("_select_below_line", [](py::handle self, std::vector<double> xcoords,
                                    std::vector<double> ycoords) {
        geo_region_select_below_line(from_cwrap<geo_region_type>(self),
                                     line_vector_data(xcoords, "xcoords"),
                                     line_vector_data(ycoords, "ycoords"));
    });
    m.def("_deselect_above_line", [](py::handle self, std::vector<double> xcoords,
                                      std::vector<double> ycoords) {
        geo_region_deselect_above_line(from_cwrap<geo_region_type>(self),
                                       line_vector_data(xcoords, "xcoords"),
                                       line_vector_data(ycoords, "ycoords"));
    });
    m.def("_deselect_below_line", [](py::handle self, std::vector<double> xcoords,
                                      std::vector<double> ycoords) {
        geo_region_deselect_below_line(from_cwrap<geo_region_type>(self),
                                       line_vector_data(xcoords, "xcoords"),
                                       line_vector_data(ycoords, "ycoords"));
    });
}
} // namespace
