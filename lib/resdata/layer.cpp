#include <cstring>
#include <cstdlib>
#include <cmath>

#include <vector>

#include <ert/util/type_macros.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/layer.hpp>

#define LAYER_TYPE_ID 55185409

struct cell_type {
    int value;
    int edges[4];
    bool bottom_barrier;
    bool left_barrier;
    bool active;
};

struct layer_struct {
    UTIL_TYPE_ID_DECLARATION;
    int nx, ny;
    std::vector<cell_type> cells;
    int cell_sum;

    [[nodiscard]] int interior_index(int i, int j) const {
        if ((i < 0) || (i >= this->nx))
            util_abort("%s: invalid i value:%d Valid range: [0,%d) \n",
                       __func__, i, this->nx);

        if ((j < 0) || (j >= this->ny))
            util_abort("%s: invalid j value:%d Valid range: [0,%d) \n",
                       __func__, j, this->ny);

        return i + j * (this->nx + 1);
    }

    [[nodiscard]] const cell_type &interior_cell(int i, int j) const {
        return this->cells[this->interior_index(i, j)];
    }

    [[nodiscard]] cell_type &interior_cell(int i, int j) {
        return this->cells[this->interior_index(i, j)];
    }
};

UTIL_IS_INSTANCE_FUNCTION(layer, LAYER_TYPE_ID)
UTIL_SAFE_CAST_FUNCTION(layer, LAYER_TYPE_ID)

layer_type *layer_alloc(int nx, int ny) {
    auto layer = new layer_type;
    UTIL_TYPE_ID_INIT(layer, LAYER_TYPE_ID);
    layer->nx = nx;
    layer->ny = ny;
    layer->cell_sum = 0;
    layer->cells = std::vector<cell_type>((layer->nx + 1) * (layer->ny + 1),
                                          {
                                              0,
                                              {0, 0, 0, 0},
                                              false,
                                              false,
                                              true,
                                          });

    return layer;
}

void layer_free(layer_type *layer) { delete layer; }

static int global_cell_index(const layer_type *layer, int i, int j) {
    if ((i < 0) || (i > layer->nx))
        util_abort("%s: invalid i value:%d Valid range: [0,%d] \n", __func__, i,
                   layer->nx);

    if ((j < 0) || (j > layer->ny))
        util_abort("%s: invalid j value:%d Valid range: [0,%d] \n", __func__, j,
                   layer->ny);

    return i + j * (layer->nx + 1);
}

bool layer_iget_left_barrier(const layer_type *layer, int i, int j) {
    return layer->interior_cell(i, j).left_barrier;
}

bool layer_iget_bottom_barrier(const layer_type *layer, int i, int j) {
    return layer->interior_cell(i, j).bottom_barrier;
}

int layer_iget_cell_value(const layer_type *layer, int i, int j) {
    return layer->interior_cell(i, j).value;
}

bool layer_iget_active(const layer_type *layer, int i, int j) {
    return layer->interior_cell(i, j).active;
}

int layer_get_cell_sum(const layer_type *layer) { return layer->cell_sum; }

static void layer_cancel_edge(layer_type *layer, int i, int j,
                              edge_dir_enum dir) {
    layer->interior_cell(i, j).edges[dir] = 0;
}

int layer_get_nx(const layer_type *layer) { return layer->nx; }

int layer_get_ny(const layer_type *layer) { return layer->ny; }

void layer_iset_cell_value(layer_type *layer, int i, int j, int value) {
    cell_type &cell = layer->interior_cell(i, j);

    layer->cell_sum += (value - cell.value);
    cell.value = value;

    if (i > 0) {
        int neighbour_value = layer_iget_cell_value(layer, i - 1, j);
        if (value == neighbour_value) {
            cell.edges[LEFT_EDGE] = 0;
            layer_cancel_edge(layer, i - 1, j, RIGHT_EDGE);
        } else
            cell.edges[LEFT_EDGE] = -value;
    } else
        cell.edges[LEFT_EDGE] = -value;

    if (i < (layer->nx - 1)) {
        int neighbour_value = layer_iget_cell_value(layer, i + 1, j);
        if (value == neighbour_value) {
            cell.edges[RIGHT_EDGE] = 0;
            layer_cancel_edge(layer, i + 1, j, LEFT_EDGE);
        } else
            cell.edges[RIGHT_EDGE] = value;
    } else
        cell.edges[RIGHT_EDGE] = value;

    if (j < (layer->ny - 1)) {
        int neighbour_value = layer_iget_cell_value(layer, i, j + 1);
        if (value == neighbour_value) {
            cell.edges[TOP_EDGE] = 0;
            layer_cancel_edge(layer, i, j + 1, BOTTOM_EDGE);
        } else
            cell.edges[TOP_EDGE] = -value;
    } else
        cell.edges[TOP_EDGE] = -value;

    if (j > 0) {
        int neighbour_value = layer_iget_cell_value(layer, i, j - 1);
        if (value == neighbour_value) {
            cell.edges[BOTTOM_EDGE] = 0;
            layer_cancel_edge(layer, i, j - 1, TOP_EDGE);
        } else
            cell.edges[BOTTOM_EDGE] = value;
    } else
        cell.edges[BOTTOM_EDGE] = value;
}

static int layer_get_global_edge_index(const layer_type *layer, int i, int j,
                                       edge_dir_enum dir) {
    if ((i < 0) || (j < 0))
        util_abort("%s: invalid value for i,j \n", __func__);

    if ((i > layer->nx) || (j > layer->ny))
        util_abort("%s: invalid value for i,j \n", __func__);

    if (i == layer->nx) {
        if (j == layer->ny)
            util_abort("%s: invalid value for i,j \n", __func__);

        if (dir != LEFT_EDGE)
            util_abort("%s: invalid value for i,j \n", __func__);
    }

    if (j == layer->ny) {
        if (i == layer->nx)
            util_abort("%s: invalid value for i,j \n", __func__);

        if (dir != BOTTOM_EDGE)
            util_abort("%s: invalid value for i,j \n", __func__);
    }

    return i + j * (layer->nx + 1);
}

int layer_iget_edge_value(const layer_type *layer, int i, int j,
                          edge_dir_enum dir) {
    return layer->cells[layer_get_global_edge_index(layer, i, j, dir)]
        .edges[dir];
}

bool layer_cell_on_edge(const layer_type *layer, int i, int j) {
    const cell_type &cell = layer->interior_cell(i, j);

    if (cell.value == cell.edges[LEFT_EDGE])
        return true;
    if (cell.value == cell.edges[RIGHT_EDGE])
        return true;
    if (cell.value == cell.edges[BOTTOM_EDGE])
        return true;
    if (cell.value == cell.edges[TOP_EDGE])
        return true;

    return false;
}

static void point_shift(int_point2d_type *point, int di, int dj) {
    point->i += di;
    point->j += dj;
}

static bool point_equal(int_point2d_type *p1, int_point2d_type *p2) {
    if ((p1->i == p2->i) && (p1->j == p2->j))
        return true;
    else
        return false;
}

/*
  Possible edge transitions:

  BOTTOM_EDGE -> BOTTOM_EDGE{ i + 1, j } , RIGHT_EDGE{i,j}     ,  LEFT_EDGE{i +1,j -1}
  RIGHT_EDGE  -> TOP_EDGE{i,j}           , RIGHT_EDGE{i,j+1}   ,  BOTTOM_EDGE{i+1 , j+1}
  TOP_EDGE    -> LEFT_EDGE{i,j}          , TOP_EDGE{i-1,j}     ,  RIGHT_EDGE{i-1,j+1}
  LEFT_EDGE   -> BOTTOM_EDGE{i,j}        , LEFT_EDGE{i,j-1}    ,  TOP_EDGE{i-1 , j-1}



*/

static void layer_trace_block_edge__(const layer_type *layer,
                                     int_point2d_type start_point, int i, int j,
                                     int value, edge_dir_enum dir,
                                     std::vector<int_point2d_type> &corner_list,
                                     int_vector_type *cell_list) {
    int_point2d_type current_point;
    int_point2d_type next_point;
    current_point.i = i;
    current_point.j = j;
    next_point = current_point;

    if (dir == BOTTOM_EDGE)
        point_shift(&next_point, 1, 0);
    else if (dir == RIGHT_EDGE) {
        point_shift(&current_point, 1, 0);
        point_shift(&next_point, 1, 1);
    } else if (dir == TOP_EDGE) {
        point_shift(&current_point, 1, 1);
        point_shift(&next_point, 0, 1);
    } else if (dir == LEFT_EDGE)
        point_shift(&current_point, 0, 1);

    corner_list.push_back(current_point);
    {
        int cell_index = i + j * layer->nx;
        int_vector_append(cell_list, cell_index);
    }

    if (!point_equal(&start_point, &next_point)) {

        if (dir == BOTTOM_EDGE) {
            if (layer_iget_edge_value(layer, i, j, RIGHT_EDGE) == value)
                layer_trace_block_edge__(layer, start_point, i, j, value,
                                         RIGHT_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i + 1, j, BOTTOM_EDGE) ==
                     value)
                layer_trace_block_edge__(layer, start_point, i + 1, j, value,
                                         BOTTOM_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i + 1, j - 1, LEFT_EDGE) ==
                     -value)
                layer_trace_block_edge__(layer, start_point, i + 1, j - 1,
                                         value, LEFT_EDGE, corner_list,
                                         cell_list);
            else
                util_abort("%s: dir == BOTTOM_EDGE \n", __func__);
        }

        if (dir == RIGHT_EDGE) {
            if (layer_iget_edge_value(layer, i, j, TOP_EDGE) == -value)
                layer_trace_block_edge__(layer, start_point, i, j, value,
                                         TOP_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i, j + 1, RIGHT_EDGE) ==
                     value)
                layer_trace_block_edge__(layer, start_point, i, j + 1, value,
                                         RIGHT_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i + 1, j + 1, BOTTOM_EDGE) ==
                     value)
                layer_trace_block_edge__(layer, start_point, i + 1, j + 1,
                                         value, BOTTOM_EDGE, corner_list,
                                         cell_list);
            else
                util_abort("%s: dir == RIGHT_EDGE \n", __func__);
        }

        if (dir == TOP_EDGE) {
            if (layer_iget_edge_value(layer, i, j, LEFT_EDGE) == -value)
                layer_trace_block_edge__(layer, start_point, i, j, value,
                                         LEFT_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i - 1, j, TOP_EDGE) == -value)
                layer_trace_block_edge__(layer, start_point, i - 1, j, value,
                                         TOP_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i - 1, j + 1, RIGHT_EDGE) ==
                     value)
                layer_trace_block_edge__(layer, start_point, i - 1, j + 1,
                                         value, RIGHT_EDGE, corner_list,
                                         cell_list);
            else
                util_abort("%s: dir == TOP_EDGE \n", __func__);
        }

        if (dir == LEFT_EDGE) {
            if (layer_iget_edge_value(layer, i, j, BOTTOM_EDGE) == value)
                layer_trace_block_edge__(layer, start_point, i, j, value,
                                         BOTTOM_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i, j - 1, LEFT_EDGE) ==
                     -value)
                layer_trace_block_edge__(layer, start_point, i, j - 1, value,
                                         LEFT_EDGE, corner_list, cell_list);
            else if (layer_iget_edge_value(layer, i - 1, j - 1, TOP_EDGE) ==
                     -value)
                layer_trace_block_edge__(layer, start_point, i - 1, j - 1,
                                         value, TOP_EDGE, corner_list,
                                         cell_list);
            else
                util_abort("%s: dir == LEFT_EDGE \n", __func__);
        }
    }
}

static bool layer_find_edge(const layer_type *layer, int *i, int *j,
                            int value) {
    const cell_type &cell = layer->interior_cell(*i, *j);
    if (cell.value == value) {

        while (!layer_cell_on_edge(layer, *i, *j))
            (*i) += 1;

        return true;
    } else
        return false;
}

bool layer_trace_block_edge(const layer_type *layer, int start_i, int start_j,
                            int value,
                            std::vector<int_point2d_type> &corner_list,
                            int_vector_type *cell_list) {
    const cell_type &cell = layer->interior_cell(start_i, start_j);
    if (cell.value == value) {
        int i = start_i;
        int j = start_j;

        if (layer_find_edge(layer, &i, &j, value)) {
            int_point2d_type start_corner;

            const cell_type &next_cell = layer->interior_cell(i, j);

            start_corner.i = i;
            start_corner.j = j;
            corner_list.clear();
            int_vector_reset(cell_list);

            if (next_cell.edges[BOTTOM_EDGE] == value) {
                point_shift(&start_corner, 0, 0);
                layer_trace_block_edge__(layer, start_corner, i, j, value,
                                         BOTTOM_EDGE, corner_list, cell_list);
            } else if (next_cell.edges[RIGHT_EDGE] == value) {
                point_shift(&start_corner, 1, 0);
                layer_trace_block_edge__(layer, start_corner, i, j, value,
                                         RIGHT_EDGE, corner_list, cell_list);
            } else if (next_cell.edges[TOP_EDGE] == -value) {
                point_shift(&start_corner, 1, 1);
                layer_trace_block_edge__(layer, start_corner, i, j, value,
                                         TOP_EDGE, corner_list, cell_list);
            } else if (next_cell.edges[LEFT_EDGE] == -value) {
                point_shift(&start_corner, 0, 1);
                layer_trace_block_edge__(layer, start_corner, i, j, value,
                                         LEFT_EDGE, corner_list, cell_list);
            } else
                util_abort("%s: Internal error \n", __func__);

            int_vector_select_unique(cell_list);
            return true;
        }
    }

    return false;
}

static void layer_trace_block_content__(layer_type *layer, bool erase, int i,
                                        int j, int value, bool *visited,
                                        int_vector_type *i_list,
                                        int_vector_type *j_list) {
    int g = layer->interior_index(i, j);
    const cell_type &cell = layer->cells[g];
    if (cell.value != value || visited[g])
        return;
    visited[g] = true;
    if (erase)
        layer_iset_cell_value(layer, i, j, 0);

    int_vector_append(i_list, i);
    int_vector_append(j_list, j);

    if (i > 0)
        layer_trace_block_content__(layer, erase, i - 1, j, value, visited,
                                    i_list, j_list);

    if (i < (layer->nx - 1))
        layer_trace_block_content__(layer, erase, i + 1, j, value, visited,
                                    i_list, j_list);

    if (j > 0)
        layer_trace_block_content__(layer, erase, i, j - 1, value, visited,
                                    i_list, j_list);

    if (j < (layer->ny - 1))
        layer_trace_block_content__(layer, erase, i, j + 1, value, visited,
                                    i_list, j_list);
}

static bool *layer_alloc_visited_mask(const layer_type *layer) {
    int total_size = (layer->nx + 1) * (layer->ny + 1);
    bool *visited = (bool *)util_malloc(total_size * sizeof *visited);
    for (int g = 0; g < total_size; g++)
        visited[g] = false;

    return visited;
}

bool layer_trace_block_content(layer_type *layer, bool erase, int start_i,
                               int start_j, int value, int_vector_type *i_list,
                               int_vector_type *j_list) {
    bool start_tracing = false;
    const cell_type &cell = layer->interior_cell(start_i, start_j);

    if ((value == 0) && (cell.value != 0))
        start_tracing = true;
    else if ((cell.value == value) && (cell.value != 0))
        start_tracing = true;

    if (start_tracing) {
        bool *visited = layer_alloc_visited_mask(layer);

        value = cell.value;
        int_vector_reset(i_list);
        int_vector_reset(j_list);
        layer_trace_block_content__(layer, erase, start_i, start_j, value,
                                    visited, i_list, j_list);

        free(visited);
        return true;
    } else
        return false;
}

int layer_replace_cell_values(layer_type *layer, int old_value, int new_value) {
    int replace_count = 0;

    for (int j = 0; j < layer->ny; j++) {
        for (int i = 0; i < layer->nx; i++) {
            if (layer_iget_cell_value(layer, i, j) == old_value) {
                layer_iset_cell_value(layer, i, j, new_value);
                replace_count++;
            }
        }
    }

    return replace_count;
}

static void layer_assert_cell_index(const layer_type *layer, int i, int j) {
    if ((i < 0) || (j < 0))
        util_abort("%s: invalid value for i,j  i:%d  [0,%d)    j:%d  [0,%d) \n",
                   __func__, i, layer->nx, j, layer->ny);

    if ((i >= layer->nx) || (j >= layer->ny))
        util_abort("%s: invalid value for i,j  i:%d  [0,%d)    j:%d  [0,%d) \n",
                   __func__, i, layer->nx, j, layer->ny);
}

bool layer_cell_contact(const layer_type *layer, int i1, int j1, int i2,
                        int j2) {
    layer_assert_cell_index(layer, i1, j1);
    layer_assert_cell_index(layer, i2, j2);
    if ((abs(i1 - i2) == 1) && (j1 == j2)) {
        int i = util_int_max(i1, i2);
        return !layer->interior_cell(i, j1).left_barrier;
    }

    if ((i1 == i2) && (abs(j1 - j2) == 1)) {
        int j = util_int_max(j1, j2);
        return !layer->interior_cell(i1, j).bottom_barrier;
    }

    return false;
}

void layer_add_ijbarrier(layer_type *layer, int i1, int j1, int i2, int j2) {
    if ((j1 == j2) || (i1 == i2)) {
        if (i1 == i2) {
            int jmin = util_int_min(j1, j2);
            int jmax = util_int_max(j1, j2);

            for (int j = jmin; j < jmax; j++) {
                cell_type &cell = layer->cells[global_cell_index(layer, i1, j)];
                cell.left_barrier = true;
            }
        } else {
            int imin = util_int_min(i1, i2);
            int imax = util_int_max(i1, i2);

            for (int i = imin; i < imax; i++) {
                cell_type &cell = layer->cells[global_cell_index(layer, i, j1)];
                cell.bottom_barrier = true;
            }
        }
    } else
        util_abort("%s: fatal error must have i1 == i2 || j1 == j2 \n",
                   __func__);
}

void layer_add_barrier(layer_type *layer, int c1, int c2) {
    int dimx = layer->nx + 1;
    int j1 = c1 / dimx;
    int i1 = c1 % dimx;

    int j2 = c2 / dimx;
    int i2 = c2 % dimx;

    layer_add_ijbarrier(layer, i1, j1, i2, j2);
}

/*
  Line is parameterized as: ax + by + c = 0
*/
static double distance_to_line(double a, double b, double c, int i, int j) {
    double x0 = 1.0 * i;
    double y0 = 1.0 * j;

    return fabs(a * x0 + b * y0 + c) / sqrt(a * a + b * b);
}

void layer_add_interp_barrier(layer_type *layer, int c1, int c2) {
    int dimx = layer->nx + 1;
    int j1 = c1 / dimx;
    int i1 = c1 % dimx;

    int j2 = c2 / dimx;
    int i2 = c2 % dimx;

    if ((j1 == j2) || (i1 == i2))
        layer_add_barrier(layer, c1, c2);
    else {
        int di = abs(i2 - i1) / (i2 - i1);
        int dj = abs(j2 - j1) / (j2 - j1);
        double a = 1.0 * (j2 - j1) / (i2 - i1);
        double b = 1.0 * (j1 - a * i1);

        int i = i1;
        int j = j1;
        int c = c1;

        while (c != c2) {
            double dx = distance_to_line(a, -1, b, i + di, j);
            double dy = distance_to_line(a, -1, b, i, j + dj);

            if (dx <= dy)
                i += di;
            else
                j += dj;

            layer_add_barrier(layer, c, i + j * dimx);
            c = i + j * dimx;
        }
    }
}

void layer_memcpy(layer_type *target_layer, const layer_type *src_layer) {
    if ((target_layer->nx == src_layer->nx) &&
        (target_layer->ny == src_layer->ny)) {
        target_layer->cells.assign(src_layer->cells.begin(),
                                   src_layer->cells.end());
        target_layer->cell_sum = src_layer->cell_sum;
    } else
        util_abort("%s: fatal error - tried to copy elements between layers of "
                   "different size\n",
                   __func__);
}

void layer_assign(layer_type *layer, int value) {
    for (int j = 0; j < layer->ny; j++) {
        for (int i = 0; i < layer->nx; i++) {
            cell_type &cell = layer->interior_cell(i, j);
            cell.value = value;
            for (int &edge : cell.edges)
                edge = 0;
        }
    }
    layer->cell_sum = value * layer->nx * layer->ny;
}

void layer_clear_cells(layer_type *layer) { layer_assign(layer, 0); }

void layer_update_connected_cells(layer_type *layer, int i, int j,
                                  int org_value, int new_value) {
    if (org_value != new_value) {
        if (layer_iget_cell_value(layer, i, j) == org_value) {
            layer_iset_cell_value(layer, i, j, new_value);

            if (i < (layer->nx - 1) &&
                layer_cell_contact(layer, i, j, i + 1, j))
                layer_update_connected_cells(layer, i + 1, j, org_value,
                                             new_value);

            if (i > 0 && layer_cell_contact(layer, i, j, i - 1, j))
                layer_update_connected_cells(layer, i - 1, j, org_value,
                                             new_value);

            if (j < (layer->ny - 1) &&
                layer_cell_contact(layer, i, j, i, j + 1))
                layer_update_connected_cells(layer, i, j + 1, org_value,
                                             new_value);

            if (j > 0 && layer_cell_contact(layer, i, j, i, j - 1))
                layer_update_connected_cells(layer, i, j - 1, org_value,
                                             new_value);
        }
    }
}

void layer_cells_equal(const layer_type *layer, int value,
                       int_vector_type *i_list, int_vector_type *j_list) {
    for (int j = 0; j < layer->ny; j++) {
        for (int i = 0; i < layer->nx; i++) {
            const cell_type &cell = layer->interior_cell(i, j);
            if (cell.value == value) {
                int_vector_append(i_list, i);
                int_vector_append(j_list, j);
            }
        }
    }
}

int layer_count_equal(const layer_type *layer, int value) {
    int num_equal = 0;
    for (int j = 0; j < layer->ny; j++) {
        for (int i = 0; i < layer->nx; i++) {
            const cell_type &cell = layer->interior_cell(i, j);
            if (cell.value == value)
                num_equal++;
        }
    }
    return num_equal;
}

void layer_update_active(layer_type *layer, const rd_grid_type *grid, int k) {
    for (int j = 0; j < rd_grid_get_ny(grid); j++) {
        for (int i = 0; i < rd_grid_get_nx(grid); i++) {
            cell_type &cell = layer->interior_cell(i, j);
            cell.active = rd_grid_cell_active3(grid, i, j, k);
        }
    }
}
