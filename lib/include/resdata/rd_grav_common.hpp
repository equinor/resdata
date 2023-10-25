#ifndef ERT_RD_GRAV_COMMON_H
#define ERT_RD_GRAV_COMMON_H

#include <stdbool.h>

#include <resdata/rd_file.hpp>
#include "detail/resdata/rd_grid_cache.hpp"

#ifdef __cplusplus
extern "C" {
#endif

bool *rd_grav_common_alloc_aquifer_cell(const rd::rd_grid_cache &grid_cache,
                                        const rd_file_type *init_file);

double rd_grav_common_eval_biot_savart(const rd::rd_grid_cache &grid_cache,
                                       rd_region_type *region,
                                       const bool *aquifer,
                                       const double *weight, double utm_x,
                                       double utm_y, double depth);

double rd_grav_common_eval_geertsma(const rd::rd_grid_cache &grid_cache,
                                    rd_region_type *region, const bool *aquifer,
                                    const double *weight, double utm_x,
                                    double utm_y, double depth,
                                    double poisson_ratio, double seabed);

#ifdef __cplusplus
}

#endif
#endif
