#include <cstdint>
#include <string>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/geometry/geo_polygon_collection.hpp>
#include <ert/util/double_vector.hpp>
#include <ert/util/int_vector.hpp>
#include <resdata/fault_block.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_fault_block, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for FaultBlock";

    m.def("_get_xc", [](py::handle self) {
        return fault_block_get_xc(from_cwrap<fault_block_type>(self));
    });
    m.def("_get_yc", [](py::handle self) {
        return fault_block_get_yc(from_cwrap<fault_block_type>(self));
    });
    m.def("_get_block_id", [](py::handle self) {
        return fault_block_get_id(from_cwrap<fault_block_type>(self));
    });
    m.def("_get_size", [](py::handle self) {
        return fault_block_get_size(from_cwrap<fault_block_type>(self));
    });
    m.def("_export_cell", [](py::handle self, int index) {
        int i = 0;
        int j = 0;
        int k = 0;
        double x = 0;
        double y = 0;
        double z = 0;
        fault_block_export_cell(from_cwrap<fault_block_type>(self), index, &i,
                                &j, &k, &x, &y, &z);
        return std::make_tuple(i, j, k, x, y, z);
    });
    m.def("_assign_to_region", [](py::handle self, int region_id) {
        fault_block_assign_to_region(from_cwrap<fault_block_type>(self),
                                     region_id);
    });
    m.def("_get_region_list", [](py::handle self) {
        const int_vector_type *regions =
            fault_block_get_region_list(from_cwrap<fault_block_type>(self));
        return IntVector().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(regions), self);
    });
    m.def("_add_cell", [](py::handle self, int i, int j) {
        fault_block_add_cell(from_cwrap<fault_block_type>(self), i, j);
    });
    m.def("_get_global_index_list", [](py::handle self) {
        const int_vector_type *global_indices =
            fault_block_get_global_index_list(
                from_cwrap<fault_block_type>(self));
        return IntVector().attr("createCReference")(
            reinterpret_cast<std::uintptr_t>(global_indices), self);
    });
    m.def("_trace_edge", [](py::handle self, py::handle x_list,
                            py::handle y_list, py::handle cell_list) {
        fault_block_trace_edge(from_cwrap<fault_block_type>(self),
                               from_cwrap<double_vector_type>(x_list),
                               from_cwrap<double_vector_type>(y_list),
                               from_cwrap<int_vector_type>(cell_list));
    });
    m.def("_get_neighbours",
          [](py::handle self, bool connected_only, py::handle polylines,
             py::handle neighbour_list) {
              fault_block_list_neighbours(
                  from_cwrap<fault_block_type>(self), connected_only,
                  from_cwrap<geo_polygon_collection_type>(polylines),
                  from_cwrap<int_vector_type>(neighbour_list));
          });
    m.def("_free", [](py::handle self) {
        fault_block_free(from_cwrap<fault_block_type>(self));
    });
}
} // namespace
