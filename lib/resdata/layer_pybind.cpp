#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/int_vector.hpp>

#include <resdata/layer.hpp>
#include <resdata/rd_grid.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_layer, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for Layer";

    m.def(
        "_alloc",
        [](int nx, int ny) {
            return reinterpret_cast<std::uintptr_t>(layer_alloc(nx, ny));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        layer_free(from_cwrap<layer_type>(self));
    });
    m.def("_get_nx", [](py::handle self) {
        return layer_get_nx(from_cwrap<layer_type>(self));
    });
    m.def("_get_ny", [](py::handle self) {
        return layer_get_ny(from_cwrap<layer_type>(self));
    });
    m.def("_set_cell", [](py::handle self, int i, int j, int value) {
        layer_iset_cell_value(from_cwrap<layer_type>(self), i, j, value);
    });
    m.def("_get_cell", [](py::handle self, int i, int j) {
        return layer_iget_cell_value(from_cwrap<layer_type>(self), i, j);
    });
    m.def("_get_bottom_barrier", [](py::handle self, int i, int j) {
        return layer_iget_bottom_barrier(from_cwrap<layer_type>(self), i, j);
    });
    m.def("_get_left_barrier", [](py::handle self, int i, int j) {
        return layer_iget_left_barrier(from_cwrap<layer_type>(self), i, j);
    });
    m.def("_cell_contact",
          [](py::handle self, int i1, int j1, int i2, int j2) {
              return layer_cell_contact(from_cwrap<layer_type>(self), i1, j1,
                                        i2, j2);
          });
    m.def("_add_barrier", [](py::handle self, int c1, int c2) {
        layer_add_barrier(from_cwrap<layer_type>(self), c1, c2);
    });
    m.def("_add_ijbarrier",
          [](py::handle self, int i1, int j1, int i2, int j2) {
              layer_add_ijbarrier(from_cwrap<layer_type>(self), i1, j1, i2,
                                  j2);
          });
    m.def("_add_interp_barrier", [](py::handle self, int c1, int c2) {
        layer_add_interp_barrier(from_cwrap<layer_type>(self), c1, c2);
    });
    m.def("_clear_cells", [](py::handle self) {
        layer_clear_cells(from_cwrap<layer_type>(self));
    });
    m.def("_assign", [](py::handle self, int value) {
        layer_assign(from_cwrap<layer_type>(self), value);
    });
    m.def("_cell_sum", [](py::handle self) {
        return layer_get_cell_sum(from_cwrap<layer_type>(self));
    });
    m.def("_update_connected",
          [](py::handle self, int i, int j, int org_value, int new_value) {
              layer_update_connected_cells(from_cwrap<layer_type>(self), i, j,
                                           org_value, new_value);
          });
    m.def("_cells_equal",
          [](py::handle self, int value, py::handle i_list, py::handle j_list) {
              layer_cells_equal(from_cwrap<layer_type>(self), value,
                                from_cwrap<int_vector_type>(i_list),
                                from_cwrap<int_vector_type>(j_list));
          });
    m.def("_count_equal", [](py::handle self, int value) {
        return layer_count_equal(from_cwrap<layer_type>(self), value);
    });
    m.def("_active_cell", [](py::handle self, int i, int j) {
        return layer_iget_active(from_cwrap<layer_type>(self), i, j);
    });
    m.def("_update_active", [](py::handle self, py::handle grid, int k) {
        layer_update_active(from_cwrap<layer_type>(self),
                            from_cwrap<rd_grid_type>(grid), k);
    });
}
} // namespace
