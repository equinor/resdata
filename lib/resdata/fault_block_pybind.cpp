#include <cstdint>
#include <memory>

#include <fmt/format.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_polygon_collection.hpp>
#include <ert/util/double_vector.hpp>
#include <ert/util/int_vector.hpp>
#include <resdata/fault_block.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {

struct FaultBlockCell {
    int i, j, k;
    double x, y, z;
};

py::object make_edge_polygon(FaultBlock &self) {
    auto x_list = make_double_vector(0, 0);
    auto y_list = make_double_vector(0, 0);
    auto cell_list = make_int_vector(0, 0);
    self.trace_edge(x_list.get(), y_list.get(), cell_list.get());

    py::object polyline = Polyline()();
    int n = double_vector_size(x_list.get());
    for (int idx = 0; idx < n; idx++)
        polyline.attr("addPoint")(double_vector_iget(x_list.get(), idx),
                                  double_vector_iget(y_list.get(), idx));
    return polyline;
}

py::object fault_block_reference(std::shared_ptr<FaultBlock> block,
                                 py::handle parent) {
    if (!block)
        return py::none();
    py::object obj = py::cast(block);
    obj.attr("_parent_layer_ref") = parent;
    return obj;
}

} // namespace

PYBIND11_MODULE(fault_block, m) {
    register_exceptions(m);
    py::class_<FaultBlockCell>(m, "FaultBlockCell")
        .def(py::init<int, int, int, double, double, double>(), py::arg("i"),
             py::arg("j"), py::arg("k"), py::arg("x"), py::arg("y"),
             py::arg("z"))
        .def_readwrite("i", &FaultBlockCell::i)
        .def_readwrite("j", &FaultBlockCell::j)
        .def_readwrite("k", &FaultBlockCell::k)
        .def_readwrite("x", &FaultBlockCell::x)
        .def_readwrite("y", &FaultBlockCell::y)
        .def_readwrite("z", &FaultBlockCell::z)
        .def("__str__", [](const FaultBlockCell &self) {
            return fmt::format("({},{})", self.i, self.j);
        });

    py::class_<FaultBlock, std::shared_ptr<FaultBlock>>(m, "FaultBlock",
                                                        py::dynamic_attr())
        .def(py::init([]() -> std::shared_ptr<FaultBlock> {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly!");
            throw py::error_already_set();
        }))
        .def("__getitem__",
             [m](FaultBlock &self, py::object index) -> py::object {
                 if (py::isinstance<py::int_>(index)) {
                     long long idx = index.cast<long long>();
                     long long len = self.get_size();
                     if (idx < 0)
                         idx += len;

                     if (idx >= 0 && idx < len) {
                         int i = 0, j = 0, k = 0;
                         double x = 0, y = 0, z = 0;
                         self.export_cell(static_cast<int>(idx), &i, &j, &k, &x,
                                          &y, &z);
                         return m.attr("FaultBlockCell")(i, j, k, x, y, z);
                     } else {
                         throw py::index_error(fmt::format(
                             "Index:{} out of range: [0,{})", idx, len));
                     }
                 } else {
                     throw py::type_error(
                         "Index:%s wrong type - integer expected");
                 }
             })
        .def("__str__",
             [](FaultBlock &self) {
                 return fmt::format("Block ID: {}", self.get_id());
             })
        .def("__len__", &FaultBlock::get_size)
        .def("get_centroid",
             [](FaultBlock &self) {
                 double xc = self.get_xc();
                 double yc = self.get_yc();
                 return py::make_tuple(xc, yc);
             })
        .def(
            "count_inside",
            [](FaultBlock &self, py::object polygon) {
                py::object point_in_polygon =
                    GeometryTools().attr("pointInPolygon");
                int inside = 0;
                int size = self.get_size();
                for (int idx = 0; idx < size; idx++) {
                    int i = 0, j = 0, k = 0;
                    double x = 0, y = 0, z = 0;
                    self.export_cell(idx, &i, &j, &k, &x, &y, &z);
                    if (point_in_polygon(py::make_tuple(x, y), polygon)
                            .cast<bool>())
                        inside += 1;
                }
                return inside;
            },
            "Will count the number of points in block which\n"
            "are inside polygon.\n",
            py::arg("polygon"))
        .def("get_block_id", &FaultBlock::get_id)
        .def("assign_to_region", &FaultBlock::assign_to_region,
             py::arg("region_id"))
        .def("get_region_list",
             [](py::object self) {
                 FaultBlock &blk = self.cast<FaultBlock &>();
                 const int_vector_type *regions = blk.get_region_list();
                 py::object region_list = IntVector().attr("createCReference")(
                     reinterpret_cast<std::uintptr_t>(regions), self);
                 return region_list.attr("copy")();
             })
        .def("add_cell", &FaultBlock::add_cell, py::arg("i"), py::arg("j"))
        .def("get_global_index_list",
             [](py::object self) {
                 FaultBlock &blk = self.cast<FaultBlock &>();
                 const int_vector_type *global_indices =
                     blk.get_global_index_list();
                 return IntVector().attr("createCReference")(
                     reinterpret_cast<std::uintptr_t>(global_indices), self);
             })
        .def("get_edge_polygon",
             [](FaultBlock &self) { return make_edge_polygon(self); })
        .def(
            "contains_polyline",
            [](FaultBlock &self, py::object polyline) {
                py::object edge_polyline = make_edge_polygon(self);
                py::object point_in_polygon =
                    GeometryTools().attr("pointInPolygon");
                for (auto p : polyline) {
                    if (point_in_polygon(p, edge_polyline).cast<bool>())
                        return true;
                }
                edge_polyline.attr("assertClosed")();
                return GeometryTools()
                    .attr("polylinesIntersect")(edge_polyline, polyline)
                    .cast<bool>();
            },
            "Will return true if at least one point from the polyline is\n"
            "inside the block.\n",
            py::arg("polyline"))
        .def(
            "get_neighbours",
            [](py::object self, py::object polylines, bool connected_only) {
                if (polylines.is_none())
                    polylines = CPolylineCollection()();

                auto &block = self.cast<FaultBlock &>();
                auto neighbours = block.get_neighbours(
                    connected_only,
                    from_cwrap<geo_polygon_collection_type>(polylines));

                py::object parent = self.attr("_parent_layer_ref");
                py::list result;
                for (const auto &neighbour : neighbours)
                    result.append(fault_block_reference(neighbour, parent));
                return result;
            },
            py::arg("polylines") = py::none(), py::arg("connected_only") = true,
            "Will return a list of FaultBlock instances which are in direct\n"
            "contact with this block.\n")
        .def("get_parent_layer",
             [](py::object self) { return self.attr("_parent_layer_ref"); });
}
