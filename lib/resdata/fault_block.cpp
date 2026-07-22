#include <vector>

#include <ert/util/int_vector.hpp>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/fault_block.hpp>
#include <resdata/fault_block_layer.hpp>
#include <resdata/layer.hpp>

#include "detail/resdata/layer_cxx.hpp"
#include "ert/util/double_vector.hpp"

struct fault_block_struct {
    rd_grid_type *grid;
    const fault_block_layer_type *parent_layer;
    int_vector_ptr i_list = make_int_vector(0, 0);
    int_vector_ptr j_list = make_int_vector(0, 0);
    int_vector_ptr global_index_list = make_int_vector(0, 0);
    int_vector_ptr region_list = make_int_vector(0, 0);
    int block_id;
    int k;
    double xc, yc;
    bool valid_center = false;
};

fault_block_type *fault_block_alloc(const fault_block_layer_type *parent_layer,
                                    int block_id) {
    auto block = new fault_block_type();
    block->parent_layer = parent_layer;
    block->grid = fault_block_layer_get_grid(parent_layer);
    block->k = fault_block_layer_get_k(parent_layer);
    block->block_id = block_id;
    return block;
}

int fault_block_get_size(const fault_block_type *block) {
    return int_vector_size(block->i_list.get());
}

int fault_block_get_id(const fault_block_type *block) {
    return block->block_id;
}

void fault_block_free(fault_block_type *block) { delete block; }

void fault_block_add_cell(fault_block_type *fault_block, int i, int j) {
    int_vector_append(fault_block->i_list.get(), i);
    int_vector_append(fault_block->j_list.get(), j);
    int_vector_append(
        fault_block->global_index_list.get(),
        rd_grid_get_global_index3(fault_block->grid, i, j, fault_block->k));
    fault_block->valid_center = false;
    layer_iset_cell_value(
        fault_block_layer_get_layer(fault_block->parent_layer), i, j,
        fault_block->block_id);
}

void fault_block_assign_to_region(fault_block_type *fault_block,
                                  int region_id) {
    if (int_vector_size(fault_block->region_list.get()) == 0)
        int_vector_append(fault_block->region_list.get(), region_id);
    else {
        if (int_vector_index_sorted(fault_block->region_list.get(),
                                    region_id) == -1)
            int_vector_append(fault_block->region_list.get(), region_id);
    }
    int_vector_sort(fault_block->region_list.get());
}

const int_vector_type *
fault_block_get_region_list(const fault_block_type *fault_block) {
    return fault_block->region_list.get();
}

static void fault_block_assert_center(fault_block_type *fault_block) {
    if (!fault_block->valid_center) {
        int index;
        double xc = 0;
        double yc = 0;

        for (index = 0; index < int_vector_size(fault_block->i_list.get());
             index++) {
            int i = int_vector_iget(fault_block->i_list.get(), index);
            int j = int_vector_iget(fault_block->j_list.get(), index);
            int g = rd_grid_get_global_index3(fault_block->grid, i, j,
                                              fault_block->k);
            double x, y, z;

            rd_grid_get_xyz1(fault_block->grid, g, &x, &y, &z);
            xc += x;
            yc += y;
        }

        fault_block->xc = xc / int_vector_size(fault_block->i_list.get());
        fault_block->yc = yc / int_vector_size(fault_block->i_list.get());
    }
    fault_block->valid_center = true;
}

double fault_block_get_xc(fault_block_type *fault_block) {
    fault_block_assert_center(fault_block);
    return fault_block->xc;
}

double fault_block_get_yc(fault_block_type *fault_block) {
    fault_block_assert_center(fault_block);
    return fault_block->yc;
}

void fault_block_export_cell(const fault_block_type *fault_block, int index,
                             int *i, int *j, int *k, double *x, double *y,
                             double *z) {
    *i = int_vector_iget(fault_block->i_list.get(), index);
    *j = int_vector_iget(fault_block->j_list.get(), index);
    *k = fault_block->k;

    rd_grid_get_xyz3(fault_block->grid, *i, *j, *k, x, y, z);
}

static int fault_block_iget_i(const fault_block_type *fault_block, int index) {
    return int_vector_iget(fault_block->i_list.get(), index);
}

static int fault_block_iget_j(const fault_block_type *fault_block, int index) {
    return int_vector_iget(fault_block->j_list.get(), index);
}

const int_vector_type *
fault_block_get_global_index_list(const fault_block_type *fault_block) {
    return fault_block->global_index_list.get();
}

bool fault_block_trace_edge(const fault_block_type *block,
                            double_vector_type *x_list,
                            double_vector_type *y_list,
                            int_vector_type *cell_list) {
    if (fault_block_get_size(block) > 0) {
        std::vector<int_point2d_type> corner_list;
        {
            int start_i = fault_block_iget_i(block, 0);
            int start_j = fault_block_iget_j(block, 0);

            layer_trace_block_edge(
                fault_block_layer_get_layer(block->parent_layer), start_i,
                start_j, block->block_id, corner_list, cell_list);
        }

        if (x_list && y_list) {
            double_vector_reset(x_list);
            double_vector_reset(y_list);
            for (const auto &p : corner_list) {
                double x, y, z;

                rd_grid_get_corner_xyz(block->grid, p.i, p.j, block->k, &x, &y,
                                       &z);
                double_vector_append(x_list, x);
                double_vector_append(y_list, y);
            }
        }

        return true;
    } else
        return false;
}

static bool
fault_block_neighbour_xpolyline(const fault_block_type *block, int i1, int j1,
                                int i2, int j2,
                                const geo_polygon_collection_type *polylines) {
    int g1 = rd_grid_get_global_index3(block->grid, i1, j1, block->k);
    int g2 = rd_grid_get_global_index3(block->grid, i2, j2, block->k);
    double x1, y1, z1;
    double x2, y2, z2;

    rd_grid_get_xyz1(block->grid, g1, &x1, &y1, &z1);
    rd_grid_get_xyz1(block->grid, g2, &x2, &y2, &z2);

    for (int i = 0; i < geo_polygon_collection_size(polylines); i++) {
        const geo_polygon_type *polyline =
            geo_polygon_collection_iget_polygon(polylines, i);
        if (geo_polygon_segment_intersects(polyline, x1, y1, x2, y2))
            return true;
    }
    return false;
}

static bool
fault_block_connected_neighbour(const fault_block_type *block, int i1, int j1,
                                int i2, int j2, bool connected_only,
                                const geo_polygon_collection_type *polylines) {
    const layer_type *layer = fault_block_layer_get_layer(block->parent_layer);
    if ((i2 < 0) || (i2 >= layer_get_nx(layer)))
        return false;

    if ((j2 < 0) || (j2 >= layer_get_ny(layer)))
        return false;

    /*
    Inactive cells do "not exist" - can not be connected neighbour
    with an inactive cell.
  */
    if (!rd_grid_cell_active3(block->grid, i2, j2, block->k))
        return false;

    {
        int cell_id = layer_iget_cell_value(layer, i1, j1);
        int neighbour_id = layer_iget_cell_value(layer, i2, j2);
        if (cell_id == neighbour_id)
            return false;

        if (!connected_only)
            return true;

        return (
            layer_cell_contact(layer, i1, j1, i2, j2) &&
            !fault_block_neighbour_xpolyline(block, i1, j1, i2, j2, polylines));
    }
}

void fault_block_list_neighbours(const fault_block_type *block,
                                 bool connected_only,
                                 const geo_polygon_collection_type *polylines,
                                 int_vector_type *neighbour_list) {
    int_vector_reset(neighbour_list);
    {
        int c;
        layer_type *layer = fault_block_layer_get_layer(block->parent_layer);
        for (c = 0; c < int_vector_size(block->i_list.get()); c++) {
            int i = int_vector_iget(block->i_list.get(), c);
            int j = int_vector_iget(block->j_list.get(), c);

            if (fault_block_connected_neighbour(block, i, j, i - 1, j,
                                                connected_only, polylines))
                int_vector_append(neighbour_list,
                                  layer_iget_cell_value(layer, i - 1, j));

            if (fault_block_connected_neighbour(block, i, j, i + 1, j,
                                                connected_only, polylines))
                int_vector_append(neighbour_list,
                                  layer_iget_cell_value(layer, i + 1, j));

            if (fault_block_connected_neighbour(block, i, j, i, j - 1,
                                                connected_only, polylines))
                int_vector_append(neighbour_list,
                                  layer_iget_cell_value(layer, i, j - 1));

            if (fault_block_connected_neighbour(block, i, j, i, j + 1,
                                                connected_only, polylines))
                int_vector_append(neighbour_list,
                                  layer_iget_cell_value(layer, i, j + 1));
        }
    }
    int_vector_select_unique(neighbour_list);
    int_vector_del_value(neighbour_list, 0);
    int_vector_del_value(neighbour_list, block->block_id);
}

void fault_block_copy_content(fault_block_type *target_block,
                              const fault_block_type *src_block) {
    for (int b = 0; b < int_vector_size(src_block->i_list.get()); b++)
        fault_block_add_cell(target_block,
                             int_vector_iget(src_block->i_list.get(), b),
                             int_vector_iget(src_block->j_list.get(), b));
}
