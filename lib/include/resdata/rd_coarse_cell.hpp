#pragma once

#include <array>
#include <memory>
#include <vector>

typedef struct rd_coarse_cell_struct rd_coarse_cell_type;
void rd_coarse_cell_free(rd_coarse_cell_type *coarse_cell);
using rd_coarse_cell_ptr =
    std::unique_ptr<rd_coarse_cell_type, decltype(&rd_coarse_cell_free)>;

bool rd_coarse_cell_equal(const rd_coarse_cell_type *coarse_cell1,
                          const rd_coarse_cell_type *coarse_cell2);
rd_coarse_cell_ptr rd_coarse_cell_alloc();
void rd_coarse_cell_update(rd_coarse_cell_type *coarse_cell, int i, int j,
                           int k, int global_index);

const std::vector<int> &
rd_coarse_cell_get_index_vector(rd_coarse_cell_type *coarse_cell);

void rd_coarse_cell_reset_active_index(rd_coarse_cell_type *coarse_cell);
void rd_coarse_cell_update_index(rd_coarse_cell_type *coarse_cell,
                                 int global_index, int *active_index,
                                 int *active_fracture_index, int active_value);
int rd_coarse_cell_get_active_index(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_get_active_fracture_index(
    const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_iget_active_cell_index(
    const rd_coarse_cell_type *coarse_cell, int index);
int rd_coarse_cell_iget_active_value(const rd_coarse_cell_type *coarse_cell,
                                     int index);
int rd_coarse_cell_get_num_active(const rd_coarse_cell_type *coarse_cell);
