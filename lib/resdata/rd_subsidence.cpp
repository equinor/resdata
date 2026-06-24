#include <cstdlib>
#include <cmath>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <optional>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/rd_subsidence.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_grav_common.hpp>
#include <resdata/rd_file_view.hpp>

#include <ert/util/type_macros.hpp>
#include <ert/util/hash.hpp>
#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>

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
    std::unique_ptr<rd::rd_grid_cache> grid_cache{nullptr};
    std::unique_ptr<bool[], decltype(&free)> aquifer_cell{nullptr, &free};
    std::unordered_map<std::string, std::unique_ptr<rd_subsidence_survey_type>>
        surveys; /* A map containing rd_subsidence_survey_type instances;
                    one instance for each interesting time. */
};

/** Data structure representing one subsidence survey. */
struct rd_subsidence_survey_struct {
    rd::rd_grid_cache *grid_cache = nullptr;
    bool *aquifer_cell =
        nullptr; /* Is this cell a numerical aquifer cell - must be disregarded. */
    std::string name;         /* Name of the survey - arbitrary string. */
    std::vector<double> porv; /* Reference pore volume */
    std::vector<double>
        pressure; /* Pressure in each grid cell at survey time */
    std::optional<std::vector<double>>
        dynamic_porevolume; /* Porevolume in each grid cell at survey time */

    rd_subsidence_survey_struct(const rd_subsidence_type &sub, const char *name)
        : grid_cache(sub.grid_cache.get()),
          aquifer_cell(sub.aquifer_cell.get()), name(name),
          porv(sub.grid_cache->size()), pressure(sub.grid_cache->size()) {}
};

static std::unique_ptr<rd_subsidence_survey_type>
rd_subsidence_survey_alloc_PRESSURE(rd_subsidence_type *rd_subsidence,
                                    const rd_file_view_type *restart_view,
                                    const char *name) {

    auto survey(
        std::make_unique<rd_subsidence_survey_struct>(*rd_subsidence, name));
    const rd::rd_grid_cache &grid_cache = *(rd_subsidence->grid_cache);
    const auto &global_index = grid_cache.global_index();
    const int size = grid_cache.size();

    rd_kw_type *init_porv_kw = rd_file_iget_named_kw(
        rd_subsidence->init_file, PORV_KW, 0); /*Global indexing*/
    rd_kw_type *pressure_kw = rd_file_view_iget_named_kw(
        restart_view, PRESSURE_KW, 0); /*Active indexing*/

    rd_kw_type *rporv_kw = nullptr;
    if (rd_file_view_has_kw(restart_view, RPORV_KW)) {
        survey->dynamic_porevolume =
            std::vector<double>(rd_subsidence->grid_cache->size(), 0.0);
        rporv_kw = rd_file_view_iget_named_kw(restart_view, RPORV_KW, 0);
    }

    for (int active_index = 0; active_index < size; active_index++) {
        survey->porv[active_index] =
            rd_kw_iget_float(init_porv_kw, global_index[active_index]);
        survey->pressure[active_index] =
            rd_kw_iget_float(pressure_kw, active_index);

        if (survey->dynamic_porevolume.has_value())
            (*survey->dynamic_porevolume)[active_index] =
                rd_kw_iget_float(rporv_kw, active_index);
    }
    return survey;
}

static double
rd_subsidence_survey_eval(const rd_subsidence_survey_type *base_survey,
                          const rd_subsidence_survey_type *monitor_survey,
                          rd_region_type *region, double utm_x, double utm_y,
                          double depth, double compressibility,
                          double poisson_ratio) {

    const rd::rd_grid_cache &grid_cache = *(base_survey->grid_cache);
    const int size = grid_cache.size();
    std::vector<double> weight(size);

    if (monitor_survey) {
        for (int index = 0; index < size; index++)
            weight[index] =
                base_survey->porv[index] * (base_survey->pressure[index] -
                                            monitor_survey->pressure[index]);
    } else {
        for (int index = 0; index < size; index++)
            weight[index] =
                base_survey->porv[index] * base_survey->pressure[index];
    }

    return compressibility * 31.83099 * (1 - poisson_ratio) *
           rd_grav_common_eval_biot_savart(grid_cache, region,
                                           base_survey->aquifer_cell,
                                           weight.data(), utm_x, utm_y, depth);
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
    std::vector<double> weight(size);

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

    return rd_grav_common_eval_geertsma(
        grid_cache, region, base_survey->aquifer_cell, weight.data(), utm_x,
        utm_y, depth, poisson_ratio, seabed);
}

static double rd_subsidence_survey_eval_geertsma_rporv(
    const rd_subsidence_survey_type *base_survey,
    const rd_subsidence_survey_type *monitor_survey, rd_region_type *region,
    double utm_x, double utm_y, double depth, double youngs_modulus,
    double poisson_ratio, double seabed) {

    const rd::rd_grid_cache &grid_cache = *(base_survey->grid_cache);
    const int size = grid_cache.size();
    std::vector<double> weight(size);

    if (!base_survey->dynamic_porevolume.has_value()) {
        util_abort(
            "%s: Keyword RPORV not defined in .UNRST file for %s. Please add "
            "RPORV keyword to output in RPTRST clause in .DATA file.\n",
            __func__, base_survey->name.c_str());
    }

    if (monitor_survey && !monitor_survey->dynamic_porevolume.has_value()) {
        util_abort(
            "%s: Keyword RPORV not defined in .UNRST file for %s. Please add "
            "RPORV keyword to output in RPTRST clause in .DATA file.\n",
            __func__, monitor_survey->name.c_str());
    }

    for (size_t index = 0; index < weight.size(); ++index) {
        if (monitor_survey)
            weight[index] = ((*base_survey->dynamic_porevolume)[index] -
                             (*monitor_survey->dynamic_porevolume)[index]) /
                            (4 * M_PI);
        else
            weight[index] =
                (*base_survey->dynamic_porevolume)[index] / (4 * M_PI);
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

rd_subsidence_type *rd_subsidence_alloc(rd_grid_type *rd_grid,
                                        const rd_file_type *init_file) {
    auto rd_subsidence = new rd_subsidence_type();
    rd_subsidence->init_file = init_file;
    rd_subsidence->grid_cache = std::make_unique<rd::rd_grid_cache>(rd_grid);
    rd_subsidence->aquifer_cell.reset(rd_grav_common_alloc_aquifer_cell(
        *(rd_subsidence->grid_cache), init_file));
    return rd_subsidence;
}

rd_subsidence_survey_type *
rd_subsidence_add_survey_PRESSURE(rd_subsidence_type *subsidence,
                                  const char *name,
                                  const rd_file_view_type *restart_view) {
    if (name == nullptr)
        throw std::invalid_argument("Name cannot be NULL");
    auto survey =
        rd_subsidence_survey_alloc_PRESSURE(subsidence, restart_view, name);
    rd_subsidence_survey_type *ret = survey.get();
    subsidence->surveys[name] = std::move(survey);
    return ret;
}

bool rd_subsidence_has_survey(const rd_subsidence_type *subsidence,
                              const char *name) {
    if (name == nullptr)
        return false;
    return subsidence->surveys.count(name) > 0;
}

double rd_subsidence_eval(const rd_subsidence_type *subsidence,
                          const char *base, const char *monitor,
                          rd_region_type *region, double utm_x, double utm_y,
                          double depth, double compressibility,
                          double poisson_ratio) {
    rd_subsidence_survey_type *base_survey = subsidence->surveys.at(base).get();
    rd_subsidence_survey_type *monitor_survey = nullptr;
    if (monitor)
        monitor_survey = subsidence->surveys.at(monitor).get();
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
    rd_subsidence_survey_type *base_survey = subsidence->surveys.at(base).get();
    rd_subsidence_survey_type *monitor_survey = nullptr;
    if (monitor)
        monitor_survey = subsidence->surveys.at(monitor).get();
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
    rd_subsidence_survey_type *base_survey = subsidence->surveys.at(base).get();
    rd_subsidence_survey_type *monitor_survey = nullptr;
    if (monitor)
        monitor_survey = subsidence->surveys.at(monitor).get();
    return rd_subsidence_survey_eval_geertsma_rporv(
        base_survey, monitor_survey, region, utm_x, utm_y, depth,
        youngs_modulus, poisson_ratio, seabed);
}

void rd_subsidence_free(rd_subsidence_type *rd_subsidence) {
    delete rd_subsidence;
}
