#ifndef ERT_ECL_GRAV_H
#define ERT_ECL_GRAV_H

#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_file_view.hpp>
#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_region.hpp>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct ecl_grav_struct ecl_grav_type;
typedef struct ecl_grav_survey_struct ecl_grav_survey_type;

void ecl_grav_free(ecl_grav_type *ecl_grav_config);
ecl_grav_type *ecl_grav_alloc(const ecl_grid_type *ecl_grid,
                              const ecl_file_type *init_file);
ecl_grav_survey_type *
ecl_grav_add_survey_FIP(ecl_grav_type *grav, const char *name,
                        const ecl_file_view_type *restart_file);
ecl_grav_survey_type *
ecl_grav_add_survey_PORMOD(ecl_grav_type *grav, const char *name,
                           const ecl_file_view_type *restart_file);
ecl_grav_survey_type *
ecl_grav_add_survey_RPORV(ecl_grav_type *grav, const char *name,
                          const ecl_file_view_type *restart_file);
ecl_grav_survey_type *
ecl_grav_add_survey_RFIP(ecl_grav_type *grav, const char *name,
                         const ecl_file_view_type *restart_file);
double ecl_grav_eval(const ecl_grav_type *grav, const char *base,
                     const char *monitor, ecl_region_type *region, double utm_x,
                     double utm_y, double depth, int phase_mask);
void ecl_grav_new_std_density(ecl_grav_type *grav, ecl_phase_enum phase,
                              double default_density);
void ecl_grav_add_std_density(ecl_grav_type *grav, ecl_phase_enum phase,
                              int pvtnum, double density);

#ifdef __cplusplus
}
#endif
#endif
