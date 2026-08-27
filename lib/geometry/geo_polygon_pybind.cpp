#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <fmt/format.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include <ert/geometry/geo_polygon.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {

std::tuple<double, double> point_xy(const py::object &point) {
    return {point[py::int_(0)].cast<double>(),
            point[py::int_(1)].cast<double>()};
}

std::string format_point(double x, double y) {
    return fmt::format("({:g},{:g})", x, y);
}

PYBIND11_MODULE(cpolyline, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for CPolyline";

    py::class_<rd::Polygon, std::shared_ptr<rd::Polygon>>(m, "CPolyline")
        .def(py::init(
                 [](std::optional<std::string> name, py::object init_points) {
                     auto polygon = std::make_shared<rd::Polygon>(name);
                     for (const py::handle &item : init_points) {
                         auto [x, y] =
                             point_xy(py::reinterpret_borrow<py::object>(item));
                         polygon->add_point(x, y);
                     }
                     return polygon;
                 }),
             py::arg("name") = py::none(), py::arg("init_points") = py::tuple())
        .def_static(
            "createFromXYZFile",
            [](const std::string &filename, std::optional<std::string> name) {
                auto polygon = rd::Polygon::load_irap(filename);
                if (name)
                    polygon->set_name(name);
                return polygon;
            },
            py::arg("filename"), py::arg("name") = py::none())
        .def("__str__",
             [](const rd::Polygon &self) {
                 std::string result =
                     self.get_name() ? *self.get_name() + " [" : "[";
                 for (size_t i = 0; i < self.size(); i++) {
                     auto [x, y] = self[i];
                     result += format_point(x, y);
                     if (i < self.size() - 1)
                         result += ",";
                 }
                 result += "]";
                 return result;
             })
        .def("__repr__", [](py::object self) { return self.attr("__str__")(); })
        .def("__len__", &rd::Polygon::size)
        .def("__getitem__",
             [](const rd::Polygon &self,
                py::int_ index) -> std::tuple<double, double> {
                 py::int_ size{self.size()};
                 if (index < py::int_{0})
                     index += size;

                 if (index < py::int_{0} || index >= size)
                     throw py::index_error(
                         fmt::format("Invalid index:{} valid range: [0,{})",
                                     index.cast<long long>(), self.size()));

                 return self[index];
             })
        .def(
            "segmentIntersects",
            [](const rd::Polygon &self, py::object p1, py::object p2) {
                auto [x1, y1] = point_xy(p1);
                auto [x2, y2] = point_xy(p2);
                return self.segment_intersects(x1, y1, x2, y2);
            },
            py::arg("p1"), py::arg("p2"))
        .def(
            "intersects",
            [](const rd::Polygon &self, py::object other) {
                if (self.size() > 1) {
                    size_t n = py::len(other);
                    for (size_t index = 1; index < n; ++index) {
                        py::int_ i{index};
                        auto p2 = other[i];
                        auto p1 = other[i - py::int_{1}];
                        auto [x1, y1] = point_xy(p1);
                        auto [x2, y2] = point_xy(p2);
                        if (self.segment_intersects(x1, y1, x2, y2))
                            return true;
                    }
                }
                return false;
            },
            py::arg("polyline"))
        .def("__iadd__",
             [](rd::Polygon &self, py::object other) {
                 size_t n = py::len(other);
                 for (size_t i = 0; i < n; ++i) {
                     auto [x, y] = point_xy(other[py::int_(i)]);
                     self.add_point(x, y);
                 }
                 return self;
             })
        .def("__add__",
             [](py::object self_obj, py::object other) {
                 py::object cls = py::type::of(self_obj);
                 py::object copy = cls(py::none(), self_obj);
                 copy.attr("__iadd__")(other);
                 return copy;
             })
        .def("__radd__",
             [](py::object self_obj, py::object other) {
                 py::object cls = py::type::of(self_obj);
                 py::object copy = cls(py::none(), other);
                 copy.attr("__iadd__")(self_obj);
                 return copy;
             })
        .def(py::self == py::self)
        .def("segmentLength",
             [](const rd::Polygon &self) {
                 if (self.size() == 0)
                     throw py::value_error(
                         "Can not measure length of zero point polyline");
                 return self.length();
             })
        .def(
            "extendToBBox",
            [](const rd::Polygon &self, py::object bbox, bool start) {
                std::tuple<double, double> p0, p1;
                if (start) {
                    p0 = self[1];
                    p1 = self[0];
                } else {
                    p0 = self[self.size() - 2];
                    p1 = self[self.size() - 1];
                }

                py::object geometry_tools = GeometryTools();
                py::object p0_tup =
                    py::make_tuple(std::get<0>(p0), std::get<1>(p0));
                py::object p1_tup =
                    py::make_tuple(std::get<0>(p1), std::get<1>(p1));
                py::object ray_dir =
                    geometry_tools.attr("lineToRay")(p0_tup, p1_tup);
                py::object intersections = geometry_tools.attr(
                    "rayPolygonIntersections")(p1_tup, ray_dir, bbox);

                if (py::len(intersections) == 0)
                    throw py::value_error(
                        "Logical error - must intersect with bounding box");

                py::object p2 = intersections[py::int_(0)][py::int_(1)];

                std::optional<std::string> name;
                if (self.get_name())
                    name = "Extend:" + *self.get_name();

                py::list points;
                points.append(p1_tup);
                points.append(p2);

                py::object cls = py::type::of<rd::Polygon>();
                return cls(name, points);
            },
            py::arg("bbox"), py::arg("start") = true)
        .def(
            "addPoint",
            [](rd::Polygon &self, double xc, double yc, bool front) {
                if (front)
                    self.add_point_front(xc, yc);
                else
                    self.add_point(xc, yc);
            },
            py::arg("xc"), py::arg("yc"), py::arg("front") = false)
        .def("getName", &rd::Polygon::get_name)
        .def("unzip",
             [](const rd::Polygon &self) {
                 std::vector<double> x, y;
                 for (size_t i = 0; i < self.size(); i++) {
                     auto [xc, yc] = self[i];
                     x.push_back(xc);
                     y.push_back(yc);
                 }
                 return std::make_tuple(x, y);
             })
        .def("unzip2", [](py::handle self) { return self.attr("unzip")(); })
        .def("connect", [](const rd::Polygon &self, py::object target) {
            auto end1 = self[0];
            auto end2 = self[self.size() - 1];

            py::object geometry_tools = GeometryTools();
            py::object end1_tup =
                py::make_tuple(std::get<0>(end1), std::get<1>(end1));
            py::object end2_tup =
                py::make_tuple(std::get<0>(end2), std::get<1>(end2));

            py::object p1 =
                geometry_tools.attr("nearestPointOnPolyline")(end1_tup, target);
            py::object p2 =
                geometry_tools.attr("nearestPointOnPolyline")(end2_tup, target);

            py::object d1 = geometry_tools.attr("distance")(p1, end1_tup);
            py::object d2 = geometry_tools.attr("distance")(p2, end2_tup);

            if (d1.cast<double>() < d2.cast<double>())
                return py::cast(std::vector<py::object>{end1_tup, p1});
            else
                return py::cast(std::vector<py::object>{end2_tup, p2});
        });
}
} // namespace
