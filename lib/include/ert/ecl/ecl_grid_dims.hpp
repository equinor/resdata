#ifndef ERT_ECL_GRID_DIMS_H
#define ERT_ECL_GRID_DIMS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ert/ecl/grid_dims.hpp>

typedef struct ecl_grid_dims_struct ecl_grid_dims_type;

ecl_grid_dims_type *ecl_grid_dims_alloc(const char *grid_file,
                                        const char *data_file);
void ecl_grid_dims_free(ecl_grid_dims_type *grid_dims);
int ecl_grid_dims_get_num_grids(const ecl_grid_dims_type *grid_dims);
const grid_dims_type *
ecl_grid_dims_iget_dims(const ecl_grid_dims_type *grid_dims, int grid_nr);

#ifdef __cplusplus
}
#endif
#endif
