#ifndef ERT_ECL_GRAV_CALC_H
#define ERT_ECL_GRAV_CALC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_file.h>

double
ecl_grav_phase_deltag(double utm_x, double utm_y, double tvd,
                      const ecl_grid_type *grid, const ecl_file_type *init_file,
                      const ecl_kw_type *sat_kw1, const ecl_kw_type *rho_kw1,
                      const ecl_kw_type *porv_kw1, const ecl_kw_type *sat_kw2,
                      const ecl_kw_type *rho_kw2, const ecl_kw_type *porv_kw2);

#ifdef __cplusplus
}
#endif
#endif
