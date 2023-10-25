#ifndef ERT_RD_COARSE_CELL_H
#define ERT_RD_COARSE_CELL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_coarse_cell_struct rd_coarse_cell_type;

bool rd_coarse_cell_equal(const rd_coarse_cell_type *coarse_cell1,
                          const rd_coarse_cell_type *coarse_cell2);
rd_coarse_cell_type *rd_coarse_cell_alloc(void);
void rd_coarse_cell_update(rd_coarse_cell_type *coarse_cell, int i, int j,
                           int k, int global_index);
void rd_coarse_cell_free(rd_coarse_cell_type *coarse_cell);
void rd_coarse_cell_free__(void *arg);

int rd_coarse_cell_get_i1(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_get_j1(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_get_k1(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_get_i2(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_get_j2(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_get_k2(const rd_coarse_cell_type *coarse_cell);
const int *rd_coarse_cell_get_box_ptr(const rd_coarse_cell_type *coarse_cell);

int rd_coarse_cell_get_size(const rd_coarse_cell_type *coarse_cell);
int rd_coarse_cell_iget_cell_index(rd_coarse_cell_type *coarse_cell,
                                   int group_index);
const int *rd_coarse_cell_get_index_ptr(rd_coarse_cell_type *coarse_cell);
const int_vector_type *
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
void rd_coarse_cell_fprintf(const rd_coarse_cell_type *coarse_cell,
                            FILE *stream);

#ifdef __cplusplus
}
#endif
#endif
