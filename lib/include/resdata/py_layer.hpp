#pragma once

#include <vector>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>

#include <resdata/layer.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
class PyLayer {
public:
    PyLayer(int nx, int ny) : layer_(layer_alloc(nx, ny), layer_free) {
    }

    PyLayer(std::shared_ptr<layer_type> layer) : layer_(layer) {
    }

    std::uintptr_t address() const {
        return reinterpret_cast<std::uintptr_t>(layer_.get());
    }

    int get_nx() const { return layer_get_nx(layer_.get()); }
    int get_ny() const { return layer_get_ny(layer_.get()); }
    int nx() const { return layer_get_nx(layer_.get()); }
    int ny() const { return layer_get_ny(layer_.get()); }

    int get_cell(py::handle index) const {
        auto [i, j] = unpack_index(index);
        return layer_iget_cell_value(layer_.get(), i, j);
    }

    void set_cell(py::handle index, int value) {
        auto [i, j] = unpack_index(index);
        layer_iset_cell_value(layer_.get(), i, j, value);
    }

    bool active_cell(int i, int j) const {
        assert_ij(i, j);
        return layer_iget_active(layer_.get(), i, j);
    }

    void update_active(const py::object &grid, int k) {
        auto grid_address = py::cast<std::uintptr_t>(grid.attr("_address")());
        auto rd_grid = reinterpret_cast<rd_grid_type *>(grid_address);
        int grid_nx = rd_grid_get_nx(rd_grid);
        int grid_ny = rd_grid_get_ny(rd_grid);
        int grid_nz = rd_grid_get_nz(rd_grid);
        if (grid_nx != get_nx()) {
            throw py::value_error("NX dimension mismatch. Grid:" +
                                  std::to_string(grid_nx) + "  layer:" +
                                  std::to_string(get_nx()));
        }
        if (grid_ny != get_ny()) {
            throw py::value_error("NY dimension mismatch. Grid:" +
                                  std::to_string(grid_ny) + "  layer:" +
                                  std::to_string(get_ny()));
        }
        if (k >= grid_nz) {
            throw py::value_error("K value invalid: Grid range [0," +
                                  std::to_string(grid_nz) + ")");
        }
        layer_update_active(layer_.get(), rd_grid, k);
    }

    bool bottom_barrier(int i, int j) const {
        assert_ij(i, j);
        return layer_iget_bottom_barrier(layer_.get(), i, j);
    }

    bool left_barrier(int i, int j) const {
        assert_ij(i, j);
        return layer_iget_left_barrier(layer_.get(), i, j);
    }

    bool cell_contact(py::tuple p1, py::tuple p2) const {
        auto [i1, j1] = unpack_point(p1);
        auto [i2, j2] = unpack_point(p2);

        if (!(0 <= i1 && i1 < get_nx())) {
            throw py::index_error("Invalid i1:" + std::to_string(i1));
        }
        if (!(0 <= i2 && i2 < get_nx())) {
            throw py::index_error("Invalid i2:" + std::to_string(i2));
        }
        if (!(0 <= j1 && j1 < get_ny())) {
            throw py::index_error("Invalid i1:" + std::to_string(j1));
        }
        if (!(0 <= j2 && j2 < get_ny())) {
            throw py::index_error("Invalid i2:" + std::to_string(j2));
        }

        return layer_cell_contact(layer_.get(), i1, j1, i2, j2);
    }

    void add_interp_barrier(int c1, int c2) {
        layer_add_interp_barrier(layer_.get(), c1, c2);
    }

    void add_polyline_barrier(const py::object &polyline, const py::object &grid,
                              int k) {
        py::ssize_t length = py::len(polyline);
        if (length <= 1) {
            return;
        }
        for (py::ssize_t i = 0; i < (length - 1); ++i) {
            auto p1 = polyline[py::int_(i)].cast<py::tuple>();
            auto p2 = polyline[py::int_(i + 1)].cast<py::tuple>();
            int c1 = py::cast<int>(
                grid.attr("findCellCornerXY")(p1[0], p1[1], py::int_(k)));
            int c2 = py::cast<int>(
                grid.attr("findCellCornerXY")(p2[0], p2[1], py::int_(k)));
            add_interp_barrier(c1, c2);
        }
    }

    void add_fault_barrier(const py::object &fault, int k, bool link_segments) {
        py::object fault_layer = fault[py::int_(k)];
        py::ssize_t num_lines = py::len(fault_layer);
        int c2 = 0;
        for (py::ssize_t index = 0; index < num_lines; ++index) {
            py::object fault_line = fault_layer[py::int_(index)];
            for (py::handle segment : fault_line) {
                auto corners = segment.attr("getCorners")().cast<py::tuple>();
                int c1 = py::cast<int>(corners[0]);
                c2 = py::cast<int>(corners[1]);
                layer_add_barrier(layer_.get(), c1, c2);
            }

            if (index < (num_lines - 1) && link_segments) {
                py::object next_segment =
                    fault_layer[py::int_(index + 1)][py::int_(0)];
                auto next_corners =
                    next_segment.attr("getCorners")().cast<py::tuple>();
                int next_c1 = py::cast<int>(next_corners[0]);
                add_interp_barrier(c2, next_c1);
            }
        }
    }

    void add_ij_barrier(const py::iterable &ij_list_obj) {
        std::vector<std::tuple<int, int>> ij_list;
        for (py::handle point : ij_list_obj) {
            auto tuple = py::cast<py::tuple>(point);
            ij_list.emplace_back(py::cast<int>(py::int_(tuple[0])),
                                 py::cast<int>(py::int_(tuple[1])));
        }

        if (ij_list.size() < 2) {
            throw py::value_error("Must have at least two (i,j) points");
        }

        int nx = get_nx();
        int ny = get_ny();
        auto [i1, j1] = ij_list[0];
        for (size_t index = 1; index < ij_list.size(); ++index) {
            auto [i2, j2] = ij_list[index];
            if (i1 == i2 || j1 == j2) {
                if (!(0 <= i2 && i2 <= nx)) {
                    throw py::value_error("i value:" + std::to_string(i1) +
                                          " invalid. Valid range: [0," +
                                          std::to_string(i2) + "] ");
                }

                if (!(0 <= j2 && j2 <= ny)) {
                    throw py::value_error("i value:" + std::to_string(j1) +
                                          " invalid. Valid range: [0," +
                                          std::to_string(j2) + "] ");
                }

                layer_add_ijbarrier(layer_.get(), i1, j1, i2, j2);
                i1 = i2;
                j1 = j2;
            } else {
                throw py::value_error("Must have i1 == i2 or j1 == j2");
            }
        }
    }

    int cell_sum() const { return layer_get_cell_sum(layer_.get()); }
    void clear_cells() { layer_clear_cells(layer_.get()); }
    void assign(int value) { layer_assign(layer_.get(), value); }

    void update_connected(py::tuple ij, int new_value,
                          std::optional<int> org_value) {
        auto [i, j] = unpack_index(ij);
        if (!org_value.has_value()) {
            org_value = layer_iget_cell_value(layer_.get(), i, j);
        }

        if (layer_iget_cell_value(layer_.get(), i, j) == org_value.value()) {
            layer_update_connected_cells(layer_.get(), i, j, org_value.value(),
                                         new_value);
        } else {
            throw py::value_error("Cell (" + std::to_string(i) + ", " +
                                  std::to_string(j) + ") is not equal to " +
                                  std::to_string(org_value.value()) + " \n");
        }
    }

    std::vector<std::tuple<int, int>> cells_equal(int value) const {
        std::vector<std::tuple<int, int>> ij_list;
        for (int j = 0; j < get_ny(); ++j) {
            for (int i = 0; i < get_nx(); ++i) {
                if (layer_iget_cell_value(layer_.get(), i, j) == value) {
                    ij_list.emplace_back(i, j);
                }
            }
        }
        return ij_list;
    }

    int count_equal(int value) const { return layer_count_equal(layer_.get(), value); }

private:
    static std::tuple<int, int> unpack_point(const py::handle &point) {
        auto tuple = py::cast<py::tuple>(point);
        return {py::cast<int>(tuple[0]), py::cast<int>(tuple[1])};
    }

    std::tuple<int, int> unpack_index(const py::handle &index) const {
        try {
            auto tuple = py::cast<py::tuple>(index);
            if (py::len(tuple) != 2) {
                throw py::value_error("");
            }
            int i = py::cast<int>(tuple[0]);
            int j = py::cast<int>(tuple[1]);
            assert_ij(i, j);
            return {i, j};
        } catch (const std::exception &) {
            throw py::value_error("Index:" +
                                  py::str(index).cast<std::string>() +
                                  " is invalid - must have two integers");
        }
    }

    void assert_ij(int i, int j) const {
        if (i < 0 || i >= get_nx()) {
            throw py::value_error("Invalid layer i:" + std::to_string(i));
        }
        if (j < 0 || j >= get_ny()) {
            throw py::value_error("Invalid layer j:" + std::to_string(j));
        }
    }

    std::shared_ptr<layer_type>  layer_;
};
