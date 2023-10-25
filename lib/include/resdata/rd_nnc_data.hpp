#ifndef RD_NNC_DATA_H
#define RD_NNC_DATA_H

#include <ert/util/type_macros.hpp>

#include <resdata/rd_nnc_geometry.hpp>
#include <resdata/rd_nnc_export.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_nnc_data_struct rd_nnc_data_type;

rd_nnc_data_type *rd_nnc_data_alloc_tran(const rd_grid_type *grid,
                                         const rd_nnc_geometry_type *nnc_geo,
                                         const rd_file_view_type *init_file);
rd_nnc_data_type *
rd_nnc_data_alloc_wat_flux(const rd_grid_type *grid,
                           const rd_nnc_geometry_type *nnc_geo,
                           const rd_file_view_type *init_file);
rd_nnc_data_type *
rd_nnc_data_alloc_oil_flux(const rd_grid_type *grid,
                           const rd_nnc_geometry_type *nnc_geo,
                           const rd_file_view_type *init_file);
rd_nnc_data_type *
rd_nnc_data_alloc_gas_flux(const rd_grid_type *grid,
                           const rd_nnc_geometry_type *nnc_geo,
                           const rd_file_view_type *init_file);
void rd_nnc_data_free(rd_nnc_data_type *data);

int rd_nnc_data_get_size(rd_nnc_data_type *data);
const double *rd_nnc_data_get_values(const rd_nnc_data_type *data);

double rd_nnc_data_iget_value(const rd_nnc_data_type *data, int index);

#ifdef __cplusplus
}
#endif
#endif
