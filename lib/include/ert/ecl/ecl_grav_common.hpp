#ifndef ERT_ECL_GRAV_COMMON_H
#define ERT_ECL_GRAV_COMMON_H

#include <stdbool.h>

#include <ert/ecl/ecl_file.hpp>
#include "detail/ecl/ecl_grid_cache.hpp"

#ifdef __cplusplus
extern "C" {
#endif

bool *ecl_grav_common_alloc_aquifer_cell(const ecl::ecl_grid_cache &grid_cache,
                                         const ecl_file_type *init_file);

double ecl_grav_common_eval_biot_savart(const ecl::ecl_grid_cache &grid_cache,
                                        ecl_region_type *region,
                                        const bool *aquifer,
                                        const double *weight, double utm_x,
                                        double utm_y, double depth);

double ecl_grav_common_eval_geertsma(const ecl::ecl_grid_cache &grid_cache,
                                     ecl_region_type *region,
                                     const bool *aquifer, const double *weight,
                                     double utm_x, double utm_y, double depth,
                                     double poisson_ratio, double seabed);

#ifdef __cplusplus
}

#endif
#endif
