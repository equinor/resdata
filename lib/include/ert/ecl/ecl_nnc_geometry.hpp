#ifndef ERT_NNC_GEOMETRY_H
#define ERT_NNC_GEOMETRY_H

#include <ert/util/type_macros.hpp>

#include <ert/ecl/ecl_grid.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ecl_nnc_geometry_struct ecl_nnc_geometry_type;
typedef struct ecl_nnc_pair_struct ecl_nnc_pair_type;

struct ecl_nnc_pair_struct {
    int grid_nr1;
    int global_index1;
    int grid_nr2;
    int global_index2;

    int input_index; /* corresponds to the input ordering of this nnc */
};

UTIL_IS_INSTANCE_HEADER(ecl_nnc_geometry);
void ecl_nnc_geometry_free(ecl_nnc_geometry_type *nnc_geo);
ecl_nnc_geometry_type *ecl_nnc_geometry_alloc(const ecl_grid_type *grid);
int ecl_nnc_geometry_size(const ecl_nnc_geometry_type *nnc_geo);
const ecl_nnc_pair_type *
ecl_nnc_geometry_iget(const ecl_nnc_geometry_type *nnc_geo, int index);
bool ecl_nnc_geometry_same_kw(const ecl_nnc_pair_type *nnc1,
                              const ecl_nnc_pair_type *nnc2);

#ifdef __cplusplus
}
#endif
#endif
