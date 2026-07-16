#ifndef ERT_RD_GRAV_H
#define ERT_RD_GRAV_H

#include <optional>
#include <string>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>

typedef struct rd_grav_struct rd_grav_type;
typedef struct rd_grav_survey_struct rd_grav_survey_type;

void rd_grav_free(rd_grav_type *rd_grav_config);

rd_grav_type *rd_grav_alloc(rd_grid_type *rd_grid, const rd::File *init_file);

rd_grav_survey_type *rd_grav_add_survey_FIP(rd_grav_type *grav,
                                            const std::string &name,
                                            rd::FileView *restart_file);

rd_grav_survey_type *rd_grav_add_survey_PORMOD(rd_grav_type *grav,
                                               const std::string &name,
                                               rd::FileView *restart_file);

rd_grav_survey_type *rd_grav_add_survey_RPORV(rd_grav_type *grav,
                                              const std::string &name,
                                              rd::FileView *restart_file);

rd_grav_survey_type *rd_grav_add_survey_RFIP(rd_grav_type *grav,
                                             const std::string &name,
                                             rd::FileView *restart_file);

double rd_grav_eval(const rd_grav_type *grav, const std::string &base,
                    const std::optional<std::string> &monitor,
                    rd_region_type *region, double utm_x, double utm_y,
                    double depth, int phase_mask);

void rd_grav_new_std_density(rd_grav_type *grav, rd_phase_enum phase,
                             double default_density);

void rd_grav_add_std_density(rd_grav_type *grav, rd_phase_enum phase,
                             int pvtnum, double density);

#endif
