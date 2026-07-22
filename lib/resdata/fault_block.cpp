#include <stdexcept>
#include <memory>
#include <cstddef>
#include <tuple>
#include <vector>
#include <set>

#include <ert/util/int_vector.hpp>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/layer.hpp>

#include "detail/resdata/layer_cxx.hpp"

FaultBlock::FaultBlock(fault_block_layer_type *parent_layer, int block_id)
    : grid(fault_block_layer_get_grid(parent_layer)),
      parent_layer(parent_layer), block_id(block_id),
      k(fault_block_layer_get_k(parent_layer)) {}

int FaultBlock::get_size() const { return int_vector_size(i_list.get()); }

int FaultBlock::get_id() const { return block_id; }

void FaultBlock::add_cell(int i, int j) {
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot add a cell on a detached fault block");
    int_vector_append(i_list.get(), i);
    int_vector_append(j_list.get(), j);
    int_vector_append(global_index_list.get(),
                      rd_grid_get_global_index3(grid, i, j, k));
    valid_center = false;
    layer_iset_cell_value(fault_block_layer_get_layer(parent_layer), i, j,
                          block_id);
}

void FaultBlock::assign_to_region(int region_id) {
    if (int_vector_size(region_list.get()) == 0)
        int_vector_append(region_list.get(), region_id);
    else {
        if (int_vector_index_sorted(region_list.get(), region_id) == -1)
            int_vector_append(region_list.get(), region_id);
    }
    int_vector_sort(region_list.get());
}

const int_vector_type *FaultBlock::get_region_list() const {
    return region_list.get();
}

void FaultBlock::assert_center() {
    if (!valid_center) {
        double new_xc = 0;
        double new_yc = 0;

        for (int index = 0; index < int_vector_size(i_list.get()); index++) {
            int i = int_vector_iget(i_list.get(), index);
            int j = int_vector_iget(j_list.get(), index);
            int g = rd_grid_get_global_index3(grid, i, j, k);
            double x, y, z;

            rd_grid_get_xyz1(grid, g, &x, &y, &z);
            new_xc += x;
            new_yc += y;
        }

        xc = new_xc / int_vector_size(i_list.get());
        yc = new_yc / int_vector_size(i_list.get());
    }
    valid_center = true;
}

double FaultBlock::get_xc() {
    assert_center();
    return xc;
}

double FaultBlock::get_yc() {
    assert_center();
    return yc;
}

FaultBlockCell FaultBlock::export_cell(int index) const {
    FaultBlockCell cell;
    cell.i = int_vector_iget(i_list.get(), index);
    cell.j = int_vector_iget(j_list.get(), index);
    cell.k = k;

    rd_grid_get_xyz3(grid, cell.i, cell.j, cell.k, &cell.x, &cell.y, &cell.z);
    return cell;
}

int FaultBlock::iget_i(int index) const {
    return int_vector_iget(i_list.get(), index);
}

int FaultBlock::iget_j(int index) const {
    return int_vector_iget(j_list.get(), index);
}

const int_vector_type *FaultBlock::get_global_index_list() const {
    return global_index_list.get();
}

std::vector<std::tuple<double, double, int>> FaultBlock::trace_edge() const {
    std::vector<std::tuple<double, double, int>> edge;
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot use trace_edge on a detached fault block");
    if (get_size() == 0)
        return edge;

    std::vector<int_point2d_type> corner_list;
    auto cell_list = make_int_vector(0, 0);
    int start_i = iget_i(0);
    int start_j = iget_j(0);

    layer_trace_block_edge(fault_block_layer_get_layer(parent_layer), start_i,
                           start_j, block_id, corner_list, cell_list.get(),
                           /*dedup_cells=*/false);

    edge.reserve(corner_list.size());
    for (std::size_t idx = 0; idx < corner_list.size(); idx++) {
        const auto &p = corner_list[idx];
        double x, y, z;

        rd_grid_get_corner_xyz(grid, p.i, p.j, k, &x, &y, &z);
        edge.emplace_back(
            x, y, int_vector_iget(cell_list.get(), static_cast<int>(idx)));
    }
    return edge;
}

bool FaultBlock::neighbour_xpolyline(
    int i1, int j1, int i2, int j2,
    const geo_polygon_collection_type *polylines) const {
    int g1 = rd_grid_get_global_index3(grid, i1, j1, k);
    int g2 = rd_grid_get_global_index3(grid, i2, j2, k);
    double x1, y1, z1;
    double x2, y2, z2;

    rd_grid_get_xyz1(grid, g1, &x1, &y1, &z1);
    rd_grid_get_xyz1(grid, g2, &x2, &y2, &z2);

    for (int i = 0; i < geo_polygon_collection_size(polylines); i++) {
        const geo_polygon_type *polyline =
            geo_polygon_collection_iget_polygon(polylines, i);
        if (geo_polygon_segment_intersects(polyline, x1, y1, x2, y2))
            return true;
    }
    return false;
}

bool FaultBlock::connected_neighbour(
    int i1, int j1, int i2, int j2, bool connected_only,
    const geo_polygon_collection_type *polylines) const {
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot use connected_neighbour on a detached fault block");
    const layer_type *layer = fault_block_layer_get_layer(parent_layer);
    if ((i2 < 0) || (i2 >= layer_get_nx(layer)))
        return false;

    if ((j2 < 0) || (j2 >= layer_get_ny(layer)))
        return false;

    /* Inactive cells do "not exist" - can not be connected neighbour
    with an inactive cell. */
    if (!rd_grid_cell_active3(grid, i2, j2, k))
        return false;

    int cell_id = layer_iget_cell_value(layer, i1, j1);
    int neighbour_id = layer_iget_cell_value(layer, i2, j2);
    if (cell_id == neighbour_id)
        return false;

    if (!connected_only)
        return true;

    return (layer_cell_contact(layer, i1, j1, i2, j2) &&
            !neighbour_xpolyline(i1, j1, i2, j2, polylines));
}

std::vector<std::shared_ptr<FaultBlock>>
FaultBlock::get_neighbours(bool connected_only,
                           const geo_polygon_collection_type *polylines) const {
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot get neighbours of a detached fault block");
    std::set<int> neighbour_ids;
    layer_type *layer = fault_block_layer_get_layer(parent_layer);
    for (int c = 0; c < int_vector_size(i_list.get()); c++) {
        int i = int_vector_iget(i_list.get(), c);
        int j = int_vector_iget(j_list.get(), c);

        if (connected_neighbour(i, j, i - 1, j, connected_only, polylines))
            neighbour_ids.insert(layer_iget_cell_value(layer, i - 1, j));

        if (connected_neighbour(i, j, i + 1, j, connected_only, polylines))
            neighbour_ids.insert(layer_iget_cell_value(layer, i + 1, j));

        if (connected_neighbour(i, j, i, j - 1, connected_only, polylines))
            neighbour_ids.insert(layer_iget_cell_value(layer, i, j - 1));

        if (connected_neighbour(i, j, i, j + 1, connected_only, polylines))
            neighbour_ids.insert(layer_iget_cell_value(layer, i, j + 1));
    }
    neighbour_ids.erase(0);
    neighbour_ids.erase(block_id);

    std::vector<std::shared_ptr<FaultBlock>> neighbours;
    neighbours.reserve(neighbour_ids.size());
    for (int id : neighbour_ids) {
        neighbours.push_back(fault_block_layer_get_block(parent_layer, id));
    }
    return neighbours;
}

void FaultBlock::copy_content(const FaultBlock &src_block) {
    for (int b = 0; b < int_vector_size(src_block.i_list.get()); b++)
        add_cell(int_vector_iget(src_block.i_list.get(), b),
                 int_vector_iget(src_block.j_list.get(), b));
}
