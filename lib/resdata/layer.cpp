#include <limits>
#include <cstddef>
#include <cmath>

#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>
#include <algorithm>
#include <array>
#include <fmt/format.h>

#include <ert/util/int_vector.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/layer.hpp>

struct Cell {
    int value;
    std::array<int, 4> edges;
    bool bottom_barrier;
    bool left_barrier;
    bool active;
};

struct layer_struct {
    size_t nx, ny;
    std::vector<Cell> cells;
    int cell_sum;

    /* Both indexers range check i and j, so the result is a valid
       non-negative offset into cells. */
    [[nodiscard]] size_t interior_index(size_t i, size_t j) const {
        if (i >= this->nx)
            throw std::out_of_range(fmt::format(
                "invalid i value:{} Valid range: [0,{})", i, this->nx));

        if (j >= this->ny)
            throw std::out_of_range(fmt::format(
                "invalid j value:{} Valid range: [0,{})", j, this->ny));

        return i + j * (this->nx + 1);
    }

    [[nodiscard]] size_t global_index(size_t i, size_t j) const {
        if (i > this->nx)
            throw std::out_of_range(fmt::format(
                "invalid i value:{} Valid range: [0,{}]", i, this->nx));

        if (j > this->ny)
            throw std::out_of_range(fmt::format(
                "invalid j value:{} Valid range: [0,{}]", j, this->ny));

        return i + j * (this->nx + 1);
    }

    [[nodiscard]] const Cell &interior_cell(size_t i, size_t j) const {
        return this->cells[this->interior_index(i, j)];
    }

    [[nodiscard]] Cell &interior_cell(size_t i, size_t j) {
        return this->cells[this->interior_index(i, j)];
    }

    [[nodiscard]] const Cell &global_cell(size_t i, size_t j) const {
        return this->cells[this->global_index(i, j)];
    }

    [[nodiscard]] Cell &global_cell(size_t i, size_t j) {
        return this->cells[this->global_index(i, j)];
    }
};

layer_type *layer_alloc(size_t nx, size_t ny) {
    /* cell_sum is an int, so the cell count must fit in an int. */
    const auto max_dim = static_cast<size_t>(std::numeric_limits<int>::max());
    if (nx > max_dim || ny > max_dim || (nx + 1) * (ny + 1) > max_dim)
        throw std::invalid_argument(
            fmt::format("layer size too large: {}x{}", nx, ny));

    layer_ptr layer{new layer_type(), &layer_free};
    layer->nx = nx;
    layer->ny = ny;
    layer->cell_sum = 0;
    layer->cells = std::vector<Cell>((nx + 1) * (ny + 1), {
                                                              0,
                                                              {0, 0, 0, 0},
                                                              false,
                                                              false,
                                                              true,
                                                          });
    return layer.release();
}

void layer_free(layer_type *layer) { delete layer; }

bool layer_iget_left_barrier(const layer_type *layer, size_t i, size_t j) {
    return layer->interior_cell(i, j).left_barrier;
}

bool layer_iget_bottom_barrier(const layer_type *layer, size_t i, size_t j) {
    return layer->interior_cell(i, j).bottom_barrier;
}

int layer_iget_cell_value(const layer_type *layer, size_t i, size_t j) {
    return layer->interior_cell(i, j).value;
}

bool layer_iget_active(const layer_type *layer, size_t i, size_t j) {
    return layer->interior_cell(i, j).active;
}

int layer_get_cell_sum(const layer_type *layer) { return layer->cell_sum; }

static void layer_cancel_edge(layer_type *layer, size_t i, size_t j,
                              edge_dir_enum dir) {
    layer->interior_cell(i, j).edges[dir] = 0;
}

size_t layer_get_nx(const layer_type *layer) { return layer->nx; }

size_t layer_get_ny(const layer_type *layer) { return layer->ny; }

void layer_iset_cell_value(layer_type *layer, size_t i, size_t j, int value) {
    Cell &cell = layer->interior_cell(i, j);

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

    if ((i + 1) < layer->nx) {
        int neighbour_value = layer_iget_cell_value(layer, i + 1, j);
        if (value == neighbour_value) {
            cell.edges[RIGHT_EDGE] = 0;
            layer_cancel_edge(layer, i + 1, j, LEFT_EDGE);
        } else
            cell.edges[RIGHT_EDGE] = value;
    } else
        cell.edges[RIGHT_EDGE] = value;

    if ((j + 1) < layer->ny) {
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

static size_t layer_get_global_edge_index(const layer_type *layer, size_t i,
                                          size_t j, edge_dir_enum dir) {
    if ((i > layer->nx) || (j > layer->ny))
        throw std::out_of_range("invalid value for i,j");

    if (i == layer->nx) {
        if (j == layer->ny)
            throw std::out_of_range("invalid value for i,j");

        if (dir != LEFT_EDGE)
            throw std::out_of_range("invalid value for i,j");
    }

    if (j == layer->ny) {
        if (i == layer->nx)
            throw std::out_of_range("invalid value for i,j");

        if (dir != BOTTOM_EDGE)
            throw std::out_of_range("invalid value for i,j");
    }

    return i + j * (layer->nx + 1);
}

int layer_iget_edge_value(const layer_type *layer, size_t i, size_t j,
                          edge_dir_enum dir) {
    return layer->cells[layer_get_global_edge_index(layer, i, j, dir)]
        .edges[dir];
}

bool layer_cell_on_edge(const layer_type *layer, size_t i, size_t j) {
    const Cell &cell = layer->interior_cell(i, j);
    return std::any_of(std::begin(cell.edges), std::end(cell.edges),
                       [&](int edge) { return cell.value == edge; });
}

static void point_shift(int_point2d_type *point, size_t di, size_t dj) {
    point->i += di;
    point->j += dj;
}

static bool point_equal(int_point2d_type *p1, int_point2d_type *p2) {
    return ((p1->i == p2->i) && (p1->j == p2->j));
}

/*
  Possible edge transitions:

  BOTTOM_EDGE -> BOTTOM_EDGE{ i + 1, j } , RIGHT_EDGE{i,j}     ,  LEFT_EDGE{i +1,j -1}
  RIGHT_EDGE  -> TOP_EDGE{i,j}           , RIGHT_EDGE{i,j+1}   ,  BOTTOM_EDGE{i+1 , j+1}
  TOP_EDGE    -> LEFT_EDGE{i,j}          , TOP_EDGE{i-1,j}     ,  RIGHT_EDGE{i-1,j+1}
  LEFT_EDGE   -> BOTTOM_EDGE{i,j}        , LEFT_EDGE{i,j-1}    ,  TOP_EDGE{i-1 , j-1}



*/

static void layer_trace_block_edge__(const layer_type *layer,
                                     int_point2d_type start_point, size_t i,
                                     size_t j, int value, edge_dir_enum dir,
                                     std::vector<int_point2d_type> &corner_list,
                                     std::vector<int> &cell_list) {
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
        int cell_index = static_cast<int>(i + j * layer->nx);
        cell_list.push_back(cell_index);
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
            else {
                if (j == 0)
                    throw std::out_of_range(
                        "dir == BOTTOM_EDGE: trace leaves the layer at j == 0");

                if (layer_iget_edge_value(layer, i + 1, j - 1, LEFT_EDGE) ==
                    -value)
                    layer_trace_block_edge__(layer, start_point, i + 1, j - 1,
                                             value, LEFT_EDGE, corner_list,
                                             cell_list);
                else
                    throw std::logic_error("dir == BOTTOM_EDGE");
            }
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
                throw std::logic_error("dir == RIGHT_EDGE");
        }

        if (dir == TOP_EDGE) {
            if (layer_iget_edge_value(layer, i, j, LEFT_EDGE) == -value)
                layer_trace_block_edge__(layer, start_point, i, j, value,
                                         LEFT_EDGE, corner_list, cell_list);
            else {
                if (i == 0)
                    throw std::out_of_range(
                        "dir == TOP_EDGE: trace leaves the layer at i == 0");

                if (layer_iget_edge_value(layer, i - 1, j, TOP_EDGE) == -value)
                    layer_trace_block_edge__(layer, start_point, i - 1, j,
                                             value, TOP_EDGE, corner_list,
                                             cell_list);
                else if (layer_iget_edge_value(layer, i - 1, j + 1,
                                               RIGHT_EDGE) == value)
                    layer_trace_block_edge__(layer, start_point, i - 1, j + 1,
                                             value, RIGHT_EDGE, corner_list,
                                             cell_list);
                else
                    throw std::logic_error("dir == TOP_EDGE");
            }
        }

        if (dir == LEFT_EDGE) {
            if (layer_iget_edge_value(layer, i, j, BOTTOM_EDGE) == value)
                layer_trace_block_edge__(layer, start_point, i, j, value,
                                         BOTTOM_EDGE, corner_list, cell_list);
            else {
                if (j == 0)
                    throw std::out_of_range(
                        "dir == LEFT_EDGE: trace leaves the layer at j == 0");

                if (layer_iget_edge_value(layer, i, j - 1, LEFT_EDGE) == -value)
                    layer_trace_block_edge__(layer, start_point, i, j - 1,
                                             value, LEFT_EDGE, corner_list,
                                             cell_list);
                else {
                    if (i == 0)
                        throw std::out_of_range("dir == LEFT_EDGE: trace "
                                                "leaves the layer at i == 0");

                    if (layer_iget_edge_value(layer, i - 1, j - 1, TOP_EDGE) ==
                        -value)
                        layer_trace_block_edge__(layer, start_point, i - 1,
                                                 j - 1, value, TOP_EDGE,
                                                 corner_list, cell_list);
                    else
                        throw std::logic_error("dir == LEFT_EDGE");
                }
            }
        }
    }
}

static bool layer_find_edge(const layer_type *layer, size_t *i, size_t *j,
                            int value) {
    const Cell &cell = layer->interior_cell(*i, *j);
    if (cell.value == value) {

        while (!layer_cell_on_edge(layer, *i, *j))
            (*i) += 1;

        return true;
    } else
        return false;
}

bool layer_trace_block_edge(const layer_type *layer, size_t start_i,
                            size_t start_j, int value,
                            std::vector<int_point2d_type> &corner_list,
                            std::vector<int> &cell_list) {
    const Cell &cell = layer->interior_cell(start_i, start_j);
    if (cell.value == value) {
        size_t i = start_i;
        size_t j = start_j;

        if (layer_find_edge(layer, &i, &j, value)) {
            int_point2d_type start_corner;

            const Cell &next_cell = layer->interior_cell(i, j);

            start_corner.i = i;
            start_corner.j = j;
            corner_list.clear();
            cell_list.clear();

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
                throw std::logic_error("Internal error");
            return true;
        }
    }

    return false;
}

class BlockTracer {
    layer_type *layer;
    bool erase;
    int value;
    std::vector<bool> visited;

public:
    std::vector<std::tuple<size_t, size_t>> indices;
    BlockTracer(layer_type *layer, bool erase, int value)
        : layer(layer), erase(erase), value(value),
          visited((layer->nx + 1) * (layer->ny + 1), false) {}

    void operator()(size_t i, size_t j) {
        size_t g = layer->interior_index(i, j);
        const Cell &cell = layer->cells[g];
        if (cell.value != value || visited[g])
            return;
        visited[g] = true;
        if (erase)
            layer_iset_cell_value(layer, i, j, 0);

        indices.emplace_back(i, j);

        if (i > 0)
            (*this)(i - 1, j);

        if ((i + 1) < layer->nx)
            (*this)(i + 1, j);

        if (j > 0)
            (*this)(i, j - 1);

        if ((j + 1) < layer->ny)
            (*this)(i, j + 1);
    }
};

std::vector<std::tuple<size_t, size_t>>
layer_trace_block_content(layer_type *layer, bool erase, size_t start_i,
                          size_t start_j, int value) {
    const Cell &cell = layer->interior_cell(start_i, start_j);

    bool start_tracing = ((value == 0) && (cell.value != 0)) ||
                         ((cell.value == value) && (cell.value != 0));
    if (!start_tracing)
        return {};

    value = cell.value;
    BlockTracer bt{layer, erase, value};
    bt(start_i, start_j);
    return std::move(bt.indices);
}

int layer_replace_cell_values(layer_type *layer, int old_value, int new_value) {
    int replace_count = 0;

    for (size_t j = 0; j < layer->ny; j++) {
        for (size_t i = 0; i < layer->nx; i++) {
            if (layer_iget_cell_value(layer, i, j) == old_value) {
                layer_iset_cell_value(layer, i, j, new_value);
                replace_count++;
            }
        }
    }

    return replace_count;
}

static void layer_assert_cell_index(const layer_type *layer, size_t i,
                                    size_t j) {
    if ((i >= layer->nx) || (j >= layer->ny))
        throw std::out_of_range(
            fmt::format("invalid value for i,j  i:{}  [0,{})    j:{}  [0,{})",
                        i, layer->nx, j, layer->ny));
}

bool layer_cell_contact(const layer_type *layer, size_t i1, size_t j1,
                        size_t i2, size_t j2) {
    layer_assert_cell_index(layer, i1, j1);
    layer_assert_cell_index(layer, i2, j2);
    if ((std::max(i1, i2) - std::min(i1, i2) == 1) && (j1 == j2))
        return !layer->interior_cell(std::max(i1, i2), j1).left_barrier;

    if ((i1 == i2) && (std::max(j1, j2) - std::min(j1, j2) == 1))
        return !layer->interior_cell(i1, std::max(j1, j2)).bottom_barrier;

    return false;
}

void layer_add_ijbarrier(layer_type *layer, size_t i1, size_t j1, size_t i2,
                         size_t j2) {
    if ((j1 == j2) || (i1 == i2)) {
        if (i1 == i2) {
            size_t jmin = std::min(j1, j2);
            size_t jmax = std::max(j1, j2);

            for (size_t j = jmin; j < jmax; j++) {
                Cell &cell = layer->global_cell(i1, j);
                cell.left_barrier = true;
            }
        } else {
            size_t imin = std::min(i1, i2);
            size_t imax = std::max(i1, i2);

            for (size_t i = imin; i < imax; i++) {
                Cell &cell = layer->global_cell(i, j1);
                cell.bottom_barrier = true;
            }
        }
    } else
        throw std::invalid_argument("must have i1 == i2 || j1 == j2");
}

void layer_add_barrier(layer_type *layer, size_t c1, size_t c2) {
    size_t dimx = layer->nx + 1;
    size_t j1 = c1 / dimx;
    size_t i1 = c1 % dimx;

    size_t j2 = c2 / dimx;
    size_t i2 = c2 % dimx;

    layer_add_ijbarrier(layer, i1, j1, i2, j2);
}

/*
  Line is parameterized as: ax + by + c = 0
*/
static double distance_to_line(double a, double b, double c, double x0,
                               double y0) {
    return fabs(a * x0 + b * y0 + c) / sqrt(a * a + b * b);
}

/* Steps one unit from @from towards @to. */
static size_t step_towards(size_t from, size_t to) {
    return (to > from) ? from + 1 : from - 1;
}

void layer_add_interp_barrier(layer_type *layer, size_t c1, size_t c2) {
    size_t dimx = layer->nx + 1;
    size_t j1 = c1 / dimx;
    size_t i1 = c1 % dimx;

    size_t j2 = c2 / dimx;
    size_t i2 = c2 % dimx;

    if ((j1 == j2) || (i1 == i2))
        layer_add_barrier(layer, c1, c2);
    else {
        const double di = (i2 > i1) ? 1.0 : -1.0;
        const double dj = (j2 > j1) ? 1.0 : -1.0;
        const double x1 = static_cast<double>(i1);
        const double y1 = static_cast<double>(j1);
        double a =
            (static_cast<double>(j2) - y1) / (static_cast<double>(i2) - x1);
        double b = y1 - a * x1;

        size_t i = i1;
        size_t j = j1;
        size_t c = c1;

        while (c != c2) {
            double x = static_cast<double>(i);
            double y = static_cast<double>(j);
            double dx = distance_to_line(a, -1, b, x + di, y);
            double dy = distance_to_line(a, -1, b, x, y + dj);

            if (dx <= dy)
                i = step_towards(i, i2);
            else
                j = step_towards(j, j2);

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
        throw std::invalid_argument(
            "cannot copy elements between layers of different size");
}

void layer_assign(layer_type *layer, int value) {
    for (size_t j = 0; j < layer->ny; j++) {
        for (size_t i = 0; i < layer->nx; i++) {
            Cell &cell = layer->interior_cell(i, j);
            cell.value = value;
            std::fill(std::begin(cell.edges), std::end(cell.edges), 0);
        }
    }
    layer->cell_sum =
        value * static_cast<int>(layer->nx) * static_cast<int>(layer->ny);
}

void layer_clear_cells(layer_type *layer) { layer_assign(layer, 0); }

void layer_update_connected_cells(layer_type *layer, size_t i, size_t j,
                                  int org_value, int new_value) {
    if (org_value != new_value) {
        if (layer_iget_cell_value(layer, i, j) == org_value) {
            layer_iset_cell_value(layer, i, j, new_value);

            if ((i + 1) < layer->nx &&
                layer_cell_contact(layer, i, j, i + 1, j))
                layer_update_connected_cells(layer, i + 1, j, org_value,
                                             new_value);

            if (i > 0 && layer_cell_contact(layer, i, j, i - 1, j))
                layer_update_connected_cells(layer, i - 1, j, org_value,
                                             new_value);

            if ((j + 1) < layer->ny &&
                layer_cell_contact(layer, i, j, i, j + 1))
                layer_update_connected_cells(layer, i, j + 1, org_value,
                                             new_value);

            if (j > 0 && layer_cell_contact(layer, i, j, i, j - 1))
                layer_update_connected_cells(layer, i, j - 1, org_value,
                                             new_value);
        }
    }
}

std::vector<std::tuple<size_t, size_t>>
layer_cells_equal(const layer_type *layer, int value) {
    std::vector<std::tuple<size_t, size_t>> cells;
    for (size_t j = 0; j < layer->ny; j++) {
        for (size_t i = 0; i < layer->nx; i++) {
            const Cell &cell = layer->interior_cell(i, j);
            if (cell.value == value) {
                cells.emplace_back(i, j);
            }
        }
    }
    return cells;
}

size_t layer_count_equal(const layer_type *layer, int value) {
    size_t num_equal = 0;
    for (size_t j = 0; j < layer->ny; j++) {
        for (size_t i = 0; i < layer->nx; i++) {
            const Cell &cell = layer->interior_cell(i, j);
            if (cell.value == value)
                num_equal++;
        }
    }
    return num_equal;
}

void layer_update_active(layer_type *layer, const rd_grid_type *grid,
                         size_t k) {
    for (size_t j = 0; j < rd_grid_get_ny(grid); j++) {
        for (size_t i = 0; i < rd_grid_get_nx(grid); i++) {
            Cell &cell = layer->interior_cell(i, j);
            cell.active = rd_grid_cell_active3(grid, i, j, k);
        }
    }
}

std::unique_ptr<layer_type, decltype(&layer_free)> make_layer(size_t nx,
                                                              size_t ny) {
    return {layer_alloc(nx, ny), layer_free};
}
