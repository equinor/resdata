#ifndef ERT_RD_SUBSIDENCE_H
#define ERT_RD_SUBSIDENCE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>

typedef struct rd_subsidence_struct rd_subsidence_type;
typedef struct rd_subsidence_survey_struct rd_subsidence_survey_type;

void rd_subsidence_free(rd_subsidence_type *rd_subsidence_config);
rd_subsidence_type *rd_subsidence_alloc(const rd_grid_type *rd_grid,
                                        const rd_file_type *init_file);
rd_subsidence_survey_type *
rd_subsidence_add_survey_PRESSURE(rd_subsidence_type *subsidence,
                                  const char *name,
                                  const rd_file_view_type *restart_view);

bool rd_subsidence_has_survey(const rd_subsidence_type *subsidence,
                              const char *name);
double rd_subsidence_eval(const rd_subsidence_type *subsidence,
                          const char *base, const char *monitor,
                          rd_region_type *region, double utm_x, double utm_y,
                          double depth, double compressibility,
                          double poisson_ratio);

double rd_subsidence_eval_geertsma(const rd_subsidence_type *subsidence,
                                   const char *base, const char *monitor,
                                   rd_region_type *region, double utm_x,
                                   double utm_y, double depth,
                                   double youngs_modulus, double poisson_ratio,
                                   double seabed);

double rd_subsidence_eval_geertsma_rporv(const rd_subsidence_type *subsidence,
                                         const char *base, const char *monitor,
                                         rd_region_type *region, double utm_x,
                                         double utm_y, double depth,
                                         double youngs_modulus,
                                         double poisson_ratio, double seabed);

#ifdef __cplusplus
}
#endif
#endif
