#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <stdexcept>

#include <ert/util/util.hpp>

#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_coarse_cell.hpp>

/*

  +---------+---------+---------+---------+---------+---------+
  | 24      | 25      | 26      | 27      | 28      | 29      |
  |   [X]   |   [X]   |   [X]   |   [X]   |         |         |
  |         |         |         |         |         |         |
  +---------+=========+=========+=========+=========+---------+
  | 18      | 19      . 20      . 21      . 22      | 23      |
  |   [X]   |         .         .   [C]   .         |         |          j
  |         |         .         .         .         |         |       /|\
  +---------+. . . . ... . . . ... . . . ... . . . .+---------+        |
  | 12      | 13      . 14      . 15      . 16      | 17      |        |
  |   [X]   |   [A]   .         .   [B]   .         |   [X]   |        |
  |         |         .         .         .         |         |        |
  +---------+. . . . ... . . . ... . . . ... . . . .+---------+        +---------> i
  |  6      |  7      .  8      .  9      . 10      | 11      |
  |         |         .         .         .         |         |
  |         |         .         .         .         |         |
  +---------+=========+=========+=========+=========+---------+
  |  0      |  1      |  2      |  3      |  4      |  5      |
  |         |   [X]   |   [X]   |   [X]   |         |         |
  |         |         |         |         |         |         |
  +---------+---------+---------+---------+---------+---------+


 - A global grid of nx = 6, ny = 5 cells. The running number inside
   the cells represents the cells global index.

 - The grid contains a coarse cell of dimensions 4 x 3, the coarse
   cell is in the global region: i \in [1,4] and j \in [1,3].

 - When solving the dynamical equations the properties of the coarse
   cell will enter as one entity, but when defining the grid initially
   one can in prinsiple have more than one active cell. The example
   above shows that the coarse region has three active cells, marked
   with [A], [B] and [C] respectively.

 - In the global grid the cells marked with [X] are active, i.e. when
   counting from zero the coarse cell will be the fourth active
   cell. This particular grid has a total of ten normal active cells
   (i.e. 10 x [X]) and one coarse cell which is active.


 - The dynamic variables like PRESSURE and SWAT are only represented
   with one element for the entire coarse cell. Static properties like
   PERMX and PORO are typically defined for all the cells in the
   entire grid, the different active cells in the coarse cell are
   stored in the active_cells vector, and when we just want one
   representative property from the the coarse cell the first (often
   only ...) element in this vector is used.

 ----


 - When we introduce coarse cells the mapping between global cell
   indices and active cell indices is not unique any longer. In the
   current grid active_index == 3 will point to the coarse cell, and
   all the cells in the coarse region will:

     - rd_grid_cell_active1( grid , global_index)      => True
     - rd_grid_get_active_index1( grid , global_index) => 3



For the example above the rd_coarse_cell datastructure would have the
following content:

  ijk           = {0 , 5 , 0 , 4 , k1 , k2 }
  active_index  = 3
  cell_list     = {7,8,9,10,13,14,15,16,19,20,21,22}
  active_cells  = {13,15,21}
  active_values = {1,1,1}   <= This can 2/3 for dual porosity.

*/

struct rd_coarse_cell_struct {
    // {i1, i2 , j1 , j2 , k1 , k2}, empty until the first update
    std::optional<std::array<size_t, 6>> ijk;
    std::optional<size_t> active_index;
    std::optional<size_t> active_fracture_index;

    bool __cell_list_sorted;
    std::vector<size_t> cell_list;
    std::vector<size_t> active_cells;
    std::vector<int> active_values;
};

rd_coarse_cell_ptr rd_coarse_cell_alloc() {
    rd_coarse_cell_ptr coarse_cell{new rd_coarse_cell_struct,
                                   rd_coarse_cell_free};

    coarse_cell->ijk = std::nullopt;
    coarse_cell->active_index = std::nullopt;
    coarse_cell->active_fracture_index = std::nullopt;
    coarse_cell->__cell_list_sorted = false;
    return coarse_cell;
}

static void rd_coarse_cell_fprintf(const rd_coarse_cell_type *coarse_cell,
                                   FILE *stream) {
    fprintf(stream, "Coarse box: \n");
    if (coarse_cell->ijk) {
        const auto &ijk = *coarse_cell->ijk;
        fprintf(stream, "   i             : %3zu - %3zu\n", ijk[0], ijk[1]);
        fprintf(stream, "   j             : %3zu - %3zu\n", ijk[2], ijk[3]);
        fprintf(stream, "   k             : %3zu - %3zu\n", ijk[4], ijk[5]);
    }
    fprintf(stream, "   active_cells  : ");
    for (size_t cell : coarse_cell->active_cells)
        fprintf(stream, "%5zu ", cell);
    fprintf(stream, "\n   active_values : ");
    for (int value : coarse_cell->active_values)
        fprintf(stream, "%5d ", value);
    fprintf(stream, "\n");
}

bool rd_coarse_cell_equal(const rd_coarse_cell_type *coarse_cell1,
                          const rd_coarse_cell_type *coarse_cell2) {
    bool equal = true;
    if (coarse_cell1->active_index != coarse_cell2->active_index)
        equal = false;

    if (coarse_cell1->active_fracture_index !=
        coarse_cell2->active_fracture_index)
        equal = false;

    if (equal)
        equal = coarse_cell1->ijk == coarse_cell2->ijk;

    if (equal)
        equal = coarse_cell1->active_cells == coarse_cell2->active_cells;

    if (equal)
        equal = coarse_cell1->active_values == coarse_cell2->active_values;

    if (equal)
        equal = coarse_cell1->cell_list == coarse_cell2->cell_list;

    if (!equal) {
        rd_coarse_cell_fprintf(coarse_cell1, stdout);
        rd_coarse_cell_fprintf(coarse_cell2, stdout);
    }

    return equal;
}

/*
  Should not be called more than once with the same arguments; that is
  not checked.
*/

void rd_coarse_cell_update(rd_coarse_cell_type *coarse_cell, size_t i, size_t j,
                           size_t k, size_t global_index) {
    if (coarse_cell->ijk) {
        auto &ijk = *coarse_cell->ijk;
        ijk[0] = std::min(ijk[0], i);
        ijk[2] = std::min(ijk[2], j);
        ijk[4] = std::min(ijk[4], k);

        ijk[1] = std::max(ijk[1], i);
        ijk[3] = std::max(ijk[3], j);
        ijk[5] = std::max(ijk[5], k);
    } else {
        coarse_cell->ijk = std::array<size_t, 6>{i, i, j, j, k, k};
    }

    coarse_cell->cell_list.push_back(global_index);
    coarse_cell->__cell_list_sorted = false;
}

void rd_coarse_cell_free(rd_coarse_cell_type *coarse_cell) {
    delete coarse_cell;
}

static void rd_coarse_cell_sort(rd_coarse_cell_type *coarse_cell) {
    if (!coarse_cell->__cell_list_sorted) {
        std::sort(coarse_cell->cell_list.begin(), coarse_cell->cell_list.end());
        coarse_cell->__cell_list_sorted = true;
    }
}

size_t rd_coarse_cell_get_size(const rd_coarse_cell_type *coarse_cell) {
    return coarse_cell->cell_list.size();
}

const std::vector<size_t> &
rd_coarse_cell_get_index_vector(rd_coarse_cell_type *coarse_cell) {
    rd_coarse_cell_sort(coarse_cell);
    return coarse_cell->cell_list;
}

const std::array<size_t, 6> &
rd_coarse_cell_get_box(const rd_coarse_cell_type *coarse_cell) {
    if (!coarse_cell->ijk)
        throw std::logic_error(
            "rd_coarse_cell_get_box: the coarse cell is empty");
    return *coarse_cell->ijk;
}

void rd_coarse_cell_reset_active_index(rd_coarse_cell_type *coarse_cell) {
    coarse_cell->active_index = std::nullopt;
    coarse_cell->active_fracture_index = std::nullopt;
}

void rd_coarse_cell_update_index(rd_coarse_cell_type *coarse_cell,
                                 size_t global_index, size_t *active_index,
                                 size_t *active_fracture_index,
                                 int active_value) {
    if (active_value & CELL_ACTIVE_MATRIX) {
        if (!coarse_cell->active_index) {
            coarse_cell->active_index = *active_index;
            (*active_index) += 1;
        }
    }

    if (active_value & CELL_ACTIVE_FRACTURE) {
        if (!coarse_cell->active_fracture_index) {
            coarse_cell->active_fracture_index = *active_fracture_index;
            (*active_fracture_index) += 1;
        }
    }

    coarse_cell->active_cells.push_back(global_index);
    coarse_cell->active_values.push_back(active_value);

    if (coarse_cell->active_values.size() > 1) {
        if (coarse_cell->active_values[coarse_cell->active_values.size() - 2] !=
            active_value)
            throw std::invalid_argument(
                "coarse cell must have "
                "all active cells have the same active value");
    }
}

std::optional<size_t>
rd_coarse_cell_get_active_index(const rd_coarse_cell_type *coarse_cell) {
    return coarse_cell->active_index;
}

std::optional<size_t> rd_coarse_cell_get_active_fracture_index(
    const rd_coarse_cell_type *coarse_cell) {
    return coarse_cell->active_fracture_index;
}

/**
   Will return the global index of the 'ith active cell in the coarse
   cell.
*/
size_t
rd_coarse_cell_iget_active_cell_index(const rd_coarse_cell_type *coarse_cell,
                                      size_t index) {
    return coarse_cell->active_cells.at(index);
}

int rd_coarse_cell_iget_active_value(const rd_coarse_cell_type *coarse_cell,
                                     size_t index) {
    return coarse_cell->active_values.at(index);
}

size_t rd_coarse_cell_get_num_active(const rd_coarse_cell_type *coarse_cell) {
    return coarse_cell->active_cells.size();
}
