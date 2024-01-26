#include <stdlib.h>
#define _USE_MATH_DEFINES // for C WINDOWS
#include <math.h>
#include <stdbool.h>

#include <ert/util/hash.hpp>
#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/rd_subsidence.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_grav_common.hpp>

#include "detail/resdata/rd_grid_cache.hpp"

/**
   This file contains datastructures for calculating changes in
   subsidence from compression of reservoirs. The main datastructure is the
   rd_subsidence_type structure (which is the only structure which is
   exported).
*/

/**
   The rd_subsidence_struct datastructure is the main structure for
   calculating the subsidence from time lapse ECLIPSE simulations.
*/

struct rd_subsidence_struct {
    const rd_file_type *
        init_file; /* The init file - a shared reference owned by calling scope. */
    rd::rd_grid_cache *grid_cache;
    bool *aquifer_cell;
    hash_type *
        surveys; /* A hash table containg rd_subsidence_survey_type instances; one instance
                                          for each interesting time. */
    double *compressibility; /*total compressibility*/
    double *poisson_ratio;
};

/**
   Data structure representing one subsidence survey.
*/

#define RD_SUBSIDENCE_SURVEY_ID 88517
struct rd_subsidence_survey_struct {
    UTIL_TYPE_ID_DECLARATION;
    const rd::rd_grid_cache *grid_cache;
    const bool *
        aquifer_cell; /* Is this cell a numerical aquifer cell - must be disregarded. */
    char *name;       /* Name of the survey - arbitrary string. */
    double *porv;     /* Reference pore volume */
    double *pressure; /* Pressure in each grid cell at survey time */
    double
        *dynamic_porevolume; /* Porevolume in each grid cell at survey time */
};

static rd_subsidence_survey_type *
rd_subsidence_survey_alloc_empty(const rd_subsidence_type *sub,
                                 const char *name) {
    rd_subsidence_survey_type *survey =
        (rd_subsidence_survey_type *)util_malloc(sizeof *survey);
    UTIL_TYPE_ID_INIT(survey, RD_SUBSIDENCE_SURVEY_ID);
    survey->grid_cache = sub->grid_cache;
    survey->aquifer_cell = sub->aquifer_cell;
    survey->name = util_alloc_string_copy(name);

    survey->porv =
        (double *)util_calloc(sub->grid_cache->size(), sizeof *survey->porv);
    survey->pressure = (double *)util_calloc(sub->grid_cache->size(),
                                             sizeof *survey->pressure);
    survey->dynamic_porevolume = NULL;

    return survey;
}

static UTIL_SAFE_CAST_FUNCTION(rd_subsidence_survey, RD_SUBSIDENCE_SURVEY_ID)

    static rd_subsidence_survey_type *rd_subsidence_survey_alloc_PRESSURE(
        rd_subsidence_type *rd_subsidence,
        const rd_file_view_type *restart_view, const char *name) {

    rd_subsidence_survey_type *survey =
        rd_subsidence_survey_alloc_empty(rd_subsidence, name);
    const rd::rd_grid_cache &grid_cache = *(rd_subsidence->grid_cache);
    const auto &global_index = grid_cache.global_index();
    const int size = grid_cache.size();

    int active_index;
    rd_kw_type *init_porv_kw = rd_file_iget_named_kw(
        rd_subsidence->init_file, PORV_KW, 0); /*Global indexing*/
    rd_kw_type *pressure_kw = rd_file_view_iget_named_kw(
        restart_view, PRESSURE_KW, 0); /*Active indexing*/

    rd_kw_type *rporv_kw = NULL;
    if (rd_file_view_has_kw(restart_view, RPORV_KW)) {
        survey->dynamic_porevolume =
            (double *)util_calloc(rd_subsidence->grid_cache->size(),
                                  sizeof *survey->dynamic_porevolume);
        rporv_kw = rd_file_view_iget_named_kw(restart_view, RPORV_KW, 0);
    }

    for (active_index = 0; active_index < size; active_index++) {
        survey->porv[active_index] =
            rd_kw_iget_float(init_porv_kw, global_index[active_index]);
        survey->pressure[active_index] =
            rd_kw_iget_float(pressure_kw, active_index);

        if (rporv_kw)
            survey->dynamic_porevolume[active_index] =
                rd_kw_iget_float(rporv_kw, active_index);
    }
    return survey;
}

static void
rd_subsidence_survey_free(rd_subsidence_survey_type *subsidence_survey) {
    free(subsidence_survey->name);
    free(subsidence_survey->porv);
    free(subsidence_survey->pressure);
    free(subsidence_survey->dynamic_porevolume);
    free(subsidence_survey);
}

static void rd_subsidence_survey_free__(void *__subsidence_survey) {
    rd_subsidence_survey_type *subsidence_survey =
        rd_subsidence_survey_safe_cast(__subsidence_survey);
    rd_subsidence_survey_free(subsidence_survey);
}

static double
rd_subsidence_survey_eval(const rd_subsidence_survey_type *base_survey,
                          const rd_subsidence_survey_type *monitor_survey,
                          rd_region_type *region, double utm_x, double utm_y,
                          double depth, double compressibility,
                          double poisson_ratio) {

    const rd::rd_grid_cache &grid_cache = *(base_survey->grid_cache);
    const int size = grid_cache.size();
    double *weight = (double *)util_calloc(size, sizeof *weight);
    double deltaz;
    int index;

    if (monitor_survey != NULL) {
        for (index = 0; index < size; index++)
            weight[index] =
                base_survey->porv[index] * (base_survey->pressure[index] -
                                            monitor_survey->pressure[index]);
    } else {
        for (index = 0; index < size; index++)
            weight[index] =
                base_survey->porv[index] * base_survey->pressure[index];
    }

    deltaz = compressibility * 31.83099 * (1 - poisson_ratio) *
             rd_grav_common_eval_biot_savart(grid_cache, region,
                                             base_survey->aquifer_cell, weight,
                                             utm_x, utm_y, depth);

    free(weight);
    return deltaz;
}

static double rd_subsidence_survey_eval_geertsma(
    const rd_subsidence_survey_type *base_survey,
    const rd_subsidence_survey_type *monitor_survey, rd_region_type *region,
    double utm_x, double utm_y, double depth, double youngs_modulus,
    double poisson_ratio, double seabed) {

    const rd::rd_grid_cache &grid_cache = *(base_survey->grid_cache);
    const auto &cell_volume = grid_cache.volume();
    const int size = grid_cache.size();
    double scale_factor = 1e4 * (1 + poisson_ratio) * (1 - 2 * poisson_ratio) /
                          (4 * M_PI * (1 - poisson_ratio) * youngs_modulus);
    double *weight = (double *)util_calloc(size, sizeof *weight);
    double deltaz;

    for (int index = 0; index < size; index++) {
        if (monitor_survey) {
            weight[index] = scale_factor * cell_volume[index] *
                            (base_survey->pressure[index] -
                             monitor_survey->pressure[index]);
        } else {
            weight[index] = scale_factor * cell_volume[index] *
                            (base_survey->pressure[index]);
        }
    }

    deltaz = rd_grav_common_eval_geertsma(
        grid_cache, region, base_survey->aquifer_cell, weight, utm_x, utm_y,
        depth, poisson_ratio, seabed);

    free(weight);
    return deltaz;
}

static double rd_subsidence_survey_eval_geertsma_rporv(
    const rd_subsidence_survey_type *base_survey,
    const rd_subsidence_survey_type *monitor_survey, rd_region_type *region,
    double utm_x, double utm_y, double depth, double youngs_modulus,
    double poisson_ratio, double seabed) {

    const rd::rd_grid_cache &grid_cache = *(base_survey->grid_cache);
    std::vector<double> weight(grid_cache.size());

    if (!base_survey->dynamic_porevolume) {
        util_abort(
            "%s: Keyword RPORV not defined in .UNRST file for %s. Please add "
            "RPORV keyword to output in RPTRST clause in .DATA file.\n",
            __func__, base_survey->name);
    }

    if (monitor_survey && !monitor_survey->dynamic_porevolume) {
        util_abort(
            "%s: Keyword RPORV not defined in .UNRST file for %s. Please add "
            "RPORV keyword to output in RPTRST clause in .DATA file.\n",
            __func__, monitor_survey->name);
    }

    for (size_t index = 0; index < weight.size(); ++index) {
        if (monitor_survey)
            weight[index] = (base_survey->dynamic_porevolume[index] -
                             monitor_survey->dynamic_porevolume[index]) /
                            (4 * M_PI);
        else
            weight[index] = base_survey->dynamic_porevolume[index] / (4 * M_PI);
    }

    return rd_grav_common_eval_geertsma(
        grid_cache, region, base_survey->aquifer_cell, weight.data(), utm_x,
        utm_y, depth, poisson_ratio, seabed);
}

/**
   The grid instance is only used during the construction phase. The
   @init_file object is used by the rd_subsidence_add_survey_XXX()
   functions; and calling scope must NOT destroy this object before
   all surveys have been added.
*/

rd_subsidence_type *rd_subsidence_alloc(const rd_grid_type *rd_grid,
                                        const rd_file_type *init_file) {
    rd_subsidence_type *rd_subsidence =
        (rd_subsidence_type *)util_malloc(sizeof *rd_subsidence);
    rd_subsidence->init_file = init_file;
    rd_subsidence->grid_cache = new rd::rd_grid_cache(rd_grid);
    rd_subsidence->aquifer_cell = rd_grav_common_alloc_aquifer_cell(
        *(rd_subsidence->grid_cache), init_file);

    rd_subsidence->surveys = hash_alloc();
    return rd_subsidence;
}

static void rd_subsidence_add_survey__(rd_subsidence_type *subsidence,
                                       const char *name,
                                       rd_subsidence_survey_type *survey) {
    hash_insert_hash_owned_ref(subsidence->surveys, name, survey,
                               rd_subsidence_survey_free__);
}

rd_subsidence_survey_type *
rd_subsidence_add_survey_PRESSURE(rd_subsidence_type *subsidence,
                                  const char *name,
                                  const rd_file_view_type *restart_view) {
    rd_subsidence_survey_type *survey =
        rd_subsidence_survey_alloc_PRESSURE(subsidence, restart_view, name);
    rd_subsidence_add_survey__(subsidence, name, survey);
    return survey;
}

bool rd_subsidence_has_survey(const rd_subsidence_type *subsidence,
                              const char *name) {
    return hash_has_key(subsidence->surveys, name);
}

static rd_subsidence_survey_type *
rd_subsidence_get_survey(const rd_subsidence_type *subsidence,
                         const char *name) {
    if (name == NULL)
        return NULL; // Calling scope must determine if this is OK?
    else
        return (rd_subsidence_survey_type *)hash_get(subsidence->surveys, name);
}

double rd_subsidence_eval(const rd_subsidence_type *subsidence,
                          const char *base, const char *monitor,
                          rd_region_type *region, double utm_x, double utm_y,
                          double depth, double compressibility,
                          double poisson_ratio) {
    rd_subsidence_survey_type *base_survey =
        rd_subsidence_get_survey(subsidence, base);
    rd_subsidence_survey_type *monitor_survey =
        rd_subsidence_get_survey(subsidence, monitor);
    return rd_subsidence_survey_eval(base_survey, monitor_survey, region, utm_x,
                                     utm_y, depth, compressibility,
                                     poisson_ratio);
}

double rd_subsidence_eval_geertsma(const rd_subsidence_type *subsidence,
                                   const char *base, const char *monitor,
                                   rd_region_type *region, double utm_x,
                                   double utm_y, double depth,
                                   double youngs_modulus, double poisson_ratio,
                                   double seabed) {
    rd_subsidence_survey_type *base_survey =
        rd_subsidence_get_survey(subsidence, base);
    rd_subsidence_survey_type *monitor_survey =
        rd_subsidence_get_survey(subsidence, monitor);
    return rd_subsidence_survey_eval_geertsma(
        base_survey, monitor_survey, region, utm_x, utm_y, depth,
        youngs_modulus, poisson_ratio, seabed);
}

double rd_subsidence_eval_geertsma_rporv(const rd_subsidence_type *subsidence,
                                         const char *base, const char *monitor,
                                         rd_region_type *region, double utm_x,
                                         double utm_y, double depth,
                                         double youngs_modulus,
                                         double poisson_ratio, double seabed) {
    rd_subsidence_survey_type *base_survey =
        rd_subsidence_get_survey(subsidence, base);
    rd_subsidence_survey_type *monitor_survey =
        rd_subsidence_get_survey(subsidence, monitor);
    return rd_subsidence_survey_eval_geertsma_rporv(
        base_survey, monitor_survey, region, utm_x, utm_y, depth,
        youngs_modulus, poisson_ratio, seabed);
}

void rd_subsidence_free(rd_subsidence_type *rd_subsidence) {
    delete rd_subsidence->grid_cache;

    free(rd_subsidence->aquifer_cell);
    hash_free(rd_subsidence->surveys);
    free(rd_subsidence);
}
