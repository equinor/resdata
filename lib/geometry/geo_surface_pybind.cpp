#include <cstdint>
#include <string>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_pointset.hpp>
#include <ert/geometry/geo_surface.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_surface, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for Surface";

    m.def(
        "_alloc",
        [](std::string filename, bool loadz) {
            return reinterpret_cast<std::uintptr_t>(
                geo_surface_fload_alloc_irap(filename, loadz));
        },
        py::return_value_policy::reference);
    m.def(
        "_new",
        [](int nx, int ny, double xinc, double yinc, double xstart,
           double ystart, double angle) {
            return reinterpret_cast<std::uintptr_t>(geo_surface_alloc_new(
                nx, ny, xinc, yinc, xstart, ystart, angle));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        geo_surface_free(from_cwrap<geo_surface_type>(self));
    });
    m.def("_get_nx", [](py::handle self) {
        return geo_surface_get_nx(from_cwrap<geo_surface_type>(self));
    });
    m.def("_get_ny", [](py::handle self) {
        return geo_surface_get_ny(from_cwrap<geo_surface_type>(self));
    });
    m.def("_iget_zvalue", [](py::handle self, int index) {
        return geo_surface_iget_zvalue(from_cwrap<geo_surface_type>(self),
                                       index);
    });
    m.def("_iset_zvalue", [](py::handle self, int index, double value) {
        geo_surface_iset_zvalue(from_cwrap<geo_surface_type>(self), index,
                                value);
    });
    m.def("_write", [](py::handle self, std::string filename) {
        geo_surface_fprintf_irap(from_cwrap<geo_surface_type>(self), filename);
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return geo_surface_equal(from_cwrap<geo_surface_type>(self),
                                 from_cwrap<geo_surface_type>(other));
    });
    m.def("_header_equal", [](py::handle self, py::handle other) {
        return geo_surface_equal_header(from_cwrap<geo_surface_type>(self),
                                        from_cwrap<geo_surface_type>(other));
    });
    m.def(
        "_copy",
        [](py::handle self, bool copy_zdata) {
            return reinterpret_cast<std::uintptr_t>(geo_surface_alloc_copy(
                from_cwrap<geo_surface_type>(self), copy_zdata));
        },
        py::return_value_policy::reference);
    m.def("_assign", [](py::handle self, double value) {
        geo_surface_assign_value(from_cwrap<geo_surface_type>(self), value);
    });
    m.def("_scale", [](py::handle self, double value) {
        geo_surface_scale(from_cwrap<geo_surface_type>(self), value);
    });
    m.def("_shift", [](py::handle self, double value) {
        geo_surface_shift(from_cwrap<geo_surface_type>(self), value);
    });
    m.def("_iadd", [](py::handle self, py::handle other) {
        geo_surface_iadd(from_cwrap<geo_surface_type>(self),
                         from_cwrap<geo_surface_type>(other));
    });
    m.def("_imul", [](py::handle self, py::handle other) {
        geo_surface_imul(from_cwrap<geo_surface_type>(self),
                         from_cwrap<geo_surface_type>(other));
    });
    m.def("_isub", [](py::handle self, py::handle other) {
        geo_surface_isub(from_cwrap<geo_surface_type>(self),
                         from_cwrap<geo_surface_type>(other));
    });
    m.def("_isqrt", [](py::handle self) {
        geo_surface_isqrt(from_cwrap<geo_surface_type>(self));
    });
    m.def("_iget_xy", [](py::handle self, int index) {
        double x = 0;
        double y = 0;
        geo_surface_iget_xy(from_cwrap<geo_surface_type>(self), index, &x, &y);
        return std::make_tuple(x, y);
    });
    m.def("_get_pointset", [](py::handle self) {
        return GeoPointset().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(
                geo_surface_get_pointset(from_cwrap<geo_surface_type>(self))),
            self);
    });
}
