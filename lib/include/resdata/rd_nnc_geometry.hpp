#ifndef ERT_NNC_GEOMETRY_H
#define ERT_NNC_GEOMETRY_H

#include <ert/util/type_macros.hpp>

#include <resdata/rd_grid.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_nnc_geometry_struct rd_nnc_geometry_type;
typedef struct rd_nnc_pair_struct rd_nnc_pair_type;

struct rd_nnc_pair_struct {
    int grid_nr1;
    int global_index1;
    int grid_nr2;
    int global_index2;

    int input_index; /* corresponds to the input ordering of this nnc */
};

UTIL_IS_INSTANCE_HEADER(rd_nnc_geometry);
void rd_nnc_geometry_free(rd_nnc_geometry_type *nnc_geo);
rd_nnc_geometry_type *rd_nnc_geometry_alloc(const rd_grid_type *grid);
int rd_nnc_geometry_size(const rd_nnc_geometry_type *nnc_geo);
const rd_nnc_pair_type *
rd_nnc_geometry_iget(const rd_nnc_geometry_type *nnc_geo, int index);
bool rd_nnc_geometry_same_kw(const rd_nnc_pair_type *nnc1,
                             const rd_nnc_pair_type *nnc2);

#ifdef __cplusplus
}
#endif
#endif
