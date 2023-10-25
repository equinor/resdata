#ifndef ERT_RD_GRAV_H
#define ERT_RD_GRAV_H

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct rd_grav_struct rd_grav_type;
typedef struct rd_grav_survey_struct rd_grav_survey_type;

void rd_grav_free(rd_grav_type *rd_grav_config);
rd_grav_type *rd_grav_alloc(const rd_grid_type *rd_grid,
                            const rd_file_type *init_file);
rd_grav_survey_type *
rd_grav_add_survey_FIP(rd_grav_type *grav, const char *name,
                       const rd_file_view_type *restart_file);
rd_grav_survey_type *
rd_grav_add_survey_PORMOD(rd_grav_type *grav, const char *name,
                          const rd_file_view_type *restart_file);
rd_grav_survey_type *
rd_grav_add_survey_RPORV(rd_grav_type *grav, const char *name,
                         const rd_file_view_type *restart_file);
rd_grav_survey_type *
rd_grav_add_survey_RFIP(rd_grav_type *grav, const char *name,
                        const rd_file_view_type *restart_file);
double rd_grav_eval(const rd_grav_type *grav, const char *base,
                    const char *monitor, rd_region_type *region, double utm_x,
                    double utm_y, double depth, int phase_mask);
void rd_grav_new_std_density(rd_grav_type *grav, rd_phase_enum phase,
                             double default_density);
void rd_grav_add_std_density(rd_grav_type *grav, rd_phase_enum phase,
                             int pvtnum, double density);

#ifdef __cplusplus
}
#endif
#endif
