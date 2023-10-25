#ifndef ERT_RD_GRID_DIMS_H
#define ERT_RD_GRID_DIMS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <resdata/grid_dims.hpp>

typedef struct rd_grid_dims_struct rd_grid_dims_type;

rd_grid_dims_type *rd_grid_dims_alloc(const char *grid_file,
                                      const char *data_file);
void rd_grid_dims_free(rd_grid_dims_type *grid_dims);
int rd_grid_dims_get_num_grids(const rd_grid_dims_type *grid_dims);
const grid_dims_type *rd_grid_dims_iget_dims(const rd_grid_dims_type *grid_dims,
                                             int grid_nr);

#ifdef __cplusplus
}
#endif
#endif
