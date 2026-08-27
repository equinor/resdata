#ifndef ERT_RD_COARSE_CELL_H
#define ERT_RD_COARSE_CELL_H

#include <cstddef>
#include <array>
#include <memory>
#include <optional>
#include <vector>

typedef struct rd_coarse_cell_struct rd_coarse_cell_type;

bool rd_coarse_cell_equal(const rd_coarse_cell_type *coarse_cell1,
                          const rd_coarse_cell_type *coarse_cell2);
rd_coarse_cell_type *rd_coarse_cell_alloc(void);
void rd_coarse_cell_update(rd_coarse_cell_type *coarse_cell, size_t i, size_t j,
                           size_t k, size_t global_index);
void rd_coarse_cell_free(rd_coarse_cell_type *coarse_cell);
void rd_coarse_cell_free__(void *arg);

/* The bounding box of the coarse cell as {i1, i2, j1, j2, k1, k2}. Only valid
   after rd_coarse_cell_update() has been called at least once. */
const std::array<size_t, 6> &
rd_coarse_cell_get_box(const rd_coarse_cell_type *coarse_cell);

size_t rd_coarse_cell_get_size(const rd_coarse_cell_type *coarse_cell);

/* The global indices of all the cells in the coarse group, sorted ascending. */
const std::vector<size_t> &
rd_coarse_cell_get_index_vector(rd_coarse_cell_type *coarse_cell);

void rd_coarse_cell_reset_active_index(rd_coarse_cell_type *coarse_cell);
void rd_coarse_cell_update_index(rd_coarse_cell_type *coarse_cell,
                                 size_t global_index, size_t *active_index,
                                 size_t *active_fracture_index,
                                 int active_value);
/* Returns nullopt if the coarse cell has not been assigned an active index. */
std::optional<size_t>
rd_coarse_cell_get_active_index(const rd_coarse_cell_type *coarse_cell);
std::optional<size_t> rd_coarse_cell_get_active_fracture_index(
    const rd_coarse_cell_type *coarse_cell);
size_t
rd_coarse_cell_iget_active_cell_index(const rd_coarse_cell_type *coarse_cell,
                                      size_t index);
int rd_coarse_cell_iget_active_value(const rd_coarse_cell_type *coarse_cell,
                                     size_t index);
size_t rd_coarse_cell_get_num_active(const rd_coarse_cell_type *coarse_cell);

using rd_coarse_cell_ptr =
    std::unique_ptr<rd_coarse_cell_type, decltype(&rd_coarse_cell_free)>;
#endif
