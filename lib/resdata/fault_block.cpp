#include <stdexcept>
#include <memory>
#include <cstddef>
#include <tuple>
#include <vector>
#include <set>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/layer.hpp>

#include "detail/resdata/layer_cxx.hpp"

FaultBlock::FaultBlock(fault_block_layer_type *parent_layer, size_t block_id)
    : grid(fault_block_layer_get_grid(parent_layer)),
      parent_layer(parent_layer), block_id(block_id),
      k(fault_block_layer_get_k(parent_layer)) {}

size_t FaultBlock::get_id() const { return block_id; }

void FaultBlock::add_cell(size_t i, size_t j) {
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot add a cell on a detached fault block");
    index_list.emplace_back(i, j);
    valid_center = false;
    layer_iset_cell_value(fault_block_layer_get_layer(parent_layer), i, j,
                          static_cast<int>(block_id));
}

void FaultBlock::assign_to_region(int region_id) { regions.insert(region_id); }

void FaultBlock::assert_center() {
    if (!valid_center) {
        double new_xc = 0;
        double new_yc = 0;

        for (const auto &[i, j] : index_list) {
            size_t g = rd_grid_get_global_index3(grid, i, j, k);
            double x, y, z;

            rd_grid_get_xyz1(grid, g, &x, &y, &z);
            new_xc += x;
            new_yc += y;
        }

        xc = new_xc / index_list.size();
        yc = new_yc / index_list.size();
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

FaultBlockCell FaultBlock::export_cell(size_t index) const {
    FaultBlockCell cell;
    const auto &[i, j] = index_list.at(index);
    cell.i = i;
    cell.j = j;
    cell.k = k;

    rd_grid_get_xyz3(grid, cell.i, cell.j, cell.k, &cell.x, &cell.y, &cell.z);
    return cell;
}

const std::vector<int> FaultBlock::get_global_index_list() const {
    std::vector<int> global_index_list;
    global_index_list.reserve(index_list.size());
    for (const auto &[i, j] : index_list)
        global_index_list.push_back(
            static_cast<int>(rd_grid_get_global_index3(grid, i, j, k)));
    return global_index_list;
}

std::vector<std::tuple<double, double, int>> FaultBlock::trace_edge() const {
    std::vector<std::tuple<double, double, int>> edge;
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot use trace_edge on a detached fault block");
    if (get_size() == 0)
        return edge;

    std::vector<int_point2d_type> corner_list;
    std::vector<int> cell_list;
    const auto &[start_i, start_j] = index_list.at(0);

    layer_trace_block_edge(fault_block_layer_get_layer(parent_layer), start_i,
                           start_j, static_cast<int>(block_id), corner_list,
                           cell_list);

    edge.reserve(corner_list.size());
    for (std::size_t idx = 0; idx < corner_list.size(); idx++) {
        const auto &p = corner_list[idx];
        double x, y, z;

        rd_grid_get_corner_xyz(grid, p.i, p.j, k, &x, &y, &z);
        edge.emplace_back(x, y, cell_list.at(idx));
    }
    return edge;
}

bool FaultBlock::neighbour_xpolyline(
    size_t i1, size_t j1, size_t i2, size_t j2,
    const geo_polygon_collection_type *polylines) const {
    size_t g1 = rd_grid_get_global_index3(grid, i1, j1, k);
    size_t g2 = rd_grid_get_global_index3(grid, i2, j2, k);
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
    size_t i1, size_t j1, size_t i2, size_t j2, bool connected_only,
    const geo_polygon_collection_type *polylines) const {
    if (this->is_detached())
        throw std::invalid_argument(
            "Cannot use connected_neighbour on a detached fault block");
    const layer_type *layer = fault_block_layer_get_layer(parent_layer);
    if ((i2 >= layer_get_nx(layer)) || (j2 >= layer_get_ny(layer)))
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
    std::set<size_t> neighbour_ids;
    layer_type *layer = fault_block_layer_get_layer(parent_layer);
    for (const auto &[i, j] : index_list) {
        const auto probe = [&](size_t i2, size_t j2) {
            if (connected_neighbour(i, j, i2, j2, connected_only, polylines))
                neighbour_ids.insert(layer_iget_cell_value(layer, i2, j2));
        };

        if (i > 0)
            probe(i - 1, j);

        probe(i + 1, j);

        if (j > 0)
            probe(i, j - 1);

        probe(i, j + 1);
    }
    neighbour_ids.erase(0);
    neighbour_ids.erase(block_id);

    std::vector<std::shared_ptr<FaultBlock>> neighbours;
    neighbours.reserve(neighbour_ids.size());
    for (int id : neighbour_ids) {
        if (id < 0)
            continue;
        neighbours.push_back(fault_block_layer_get_block(parent_layer, id));
    }
    return neighbours;
}

void FaultBlock::copy_content(const FaultBlock &src_block) {
    for (const auto &[i, j] : src_block.index_list)
        add_cell(i, j);
}
