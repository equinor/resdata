#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

#include <ert/util/type_macros.hpp>

#include <resdata/rd_grid.hpp>

/*
  The elements in this enum are (ab)used as indexes into a int[] vector;
  i.e. they must span the values 0..3.
*/
typedef enum {
    RIGHT_EDGE = 0,
    LEFT_EDGE = 1,
    TOP_EDGE = 2,
    BOTTOM_EDGE = 3
} edge_dir_enum;

typedef struct {
    size_t i;
    size_t j;
} int_point2d_type;

typedef struct layer_struct layer_type;

bool layer_iget_left_barrier(const layer_type *layer, size_t i, size_t j);
bool layer_iget_bottom_barrier(const layer_type *layer, size_t i, size_t j);
size_t layer_get_nx(const layer_type *layer);
size_t layer_get_ny(const layer_type *layer);
layer_type *layer_alloc(size_t nx, size_t ny);
void layer_free(layer_type *layer);
int layer_replace_cell_values(layer_type *layer, int old_value, int new_value);
bool layer_iget_active(const layer_type *layer, size_t i, size_t j);
int layer_iget_cell_value(const layer_type *layer, size_t i, size_t j);
void layer_iset_cell_value(layer_type *layer, size_t i, size_t j, int value);
int layer_iget_edge_value(const layer_type *layer, size_t i, size_t j,
                          edge_dir_enum dir);
bool layer_cell_on_edge(const layer_type *layer, size_t i, size_t j);
int layer_get_cell_sum(const layer_type *layer);
std::vector<std::tuple<size_t, size_t>>
layer_trace_block_content(layer_type *layer, bool erase, size_t start_i,
                          size_t start_j, int value);
bool layer_cell_contact(const layer_type *layer, size_t i1, size_t j1,
                        size_t i2, size_t j2);
void layer_add_interp_barrier(layer_type *layer, size_t c1, size_t c2);
void layer_add_ijbarrier(layer_type *layer, size_t i1, size_t j1, size_t i2,
                         size_t j2);
void layer_add_barrier(layer_type *layer, size_t c1, size_t c2);
void layer_memcpy(layer_type *target_layer, const layer_type *src_layer);
void layer_update_active(layer_type *layer, const rd_grid_type *grid, size_t k);
void layer_clear_cells(layer_type *layer);
void layer_update_connected_cells(layer_type *layer, size_t i, size_t j,
                                  int org_value, int new_value);
void layer_assign(layer_type *layer, int value);

std::vector<std::tuple<size_t, size_t>>
layer_cells_equal(const layer_type *layer, int value);
size_t layer_count_equal(const layer_type *layer, int value);

using layer_ptr = std::unique_ptr<layer_type, decltype(&layer_free)>;
layer_ptr make_layer(size_t, size_t);
