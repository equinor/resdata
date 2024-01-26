#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <unordered_map>
#include <vector>
#include <string>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/rd_grav.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_grav_common.hpp>

#include "detail/resdata/rd_grid_cache.hpp"

/**
   This file contains datastructures for calculating changes in
   gravitational response in reservoirs. The main datastructure is the
   rd_grav_type structure (which is the only structure which is
   exported).
*/

#define GRAV_CALC_USE_PORV 128
#define GRAV_CALC_USE_RHO                                                      \
    256 // The GRAV_CALC_USE_RHO value is currently not used.

typedef enum {
    GRAV_CALC_RPORV = 1 + GRAV_CALC_USE_PORV + GRAV_CALC_USE_RHO,
    GRAV_CALC_PORMOD = 2 + GRAV_CALC_USE_PORV + GRAV_CALC_USE_RHO,
    GRAV_CALC_FIP = 3,
    GRAV_CALC_RFIP = 4 + GRAV_CALC_USE_RHO
} grav_calc_type;

typedef struct rd_grav_phase_struct rd_grav_phase_type;

/**
   The rd_grav_struct datastructure is the main structure for
   calculating the gravimetric response from time lapse ECLIPSE
   simulations.
*/

struct rd_grav_struct {
    const rd_file_type *
        init_file; /* The init file - a shared reference owned by calling scope. */
    rd::rd_grid_cache *
        grid_cache; /* An internal specialized structure to facilitate fast grid lookup. */
    bool *aquifer_cell; /* Numerical aquifer cells should be ignored. */

    std::unordered_map<std::string, rd_grav_survey_type *> surveys;
    std::unordered_map<std::string, double> default_density;
    std::unordered_map<std::string, std::vector<double>> std_density;
};

/**
   Data structure representing one gravimetric survey.
*/

#define RD_GRAV_SURVEY_ID 88517
struct rd_grav_survey_struct {
    UTIL_TYPE_ID_DECLARATION;
    const rd::rd_grid_cache *grid_cache;
    const bool *aquifer_cell;
    char *name; /* Name of the survey - arbitrary string. */
    double *
        porv; /* Reference shared by the rd_grav_phase structures - i.e. it must not be updated. */
    std::vector<rd_grav_phase_type *>
        phase_list; /* rd_grav_phase_type objects - one for each phase present in the model. */
    std::unordered_map<std::string, rd_grav_phase_type *>
        phase_map; /* The same objects as in the phase_list vector - accessible by the "SWAT", "SGAS" and "SOIL" keys. */
};

/**
   Data structure representing the results from one phase at one survey.
*/

#define RD_GRAV_PHASE_TYPE_ID 1066652
struct rd_grav_phase_struct {
    UTIL_TYPE_ID_DECLARATION;
    const rd::rd_grid_cache *grid_cache;
    const bool *aquifer_cell;
    double *
        fluid_mass; /* The total fluid in place (mass) of this phase - for each active cell.*/
    double *work; /* Temporary used in the summation over all cells. */
    rd_phase_enum phase;
};

static void rd_grav_phase_free(rd_grav_phase_type *grav_phase) {
    free(grav_phase->work);
    free(grav_phase->fluid_mass);
    delete grav_phase;
}

static void rd_grav_survey_free(rd_grav_survey_type *grav_survey) {
    free(grav_survey->name);
    free(grav_survey->porv);
    for (auto *phase : grav_survey->phase_list)
        rd_grav_phase_free(phase);

    delete grav_survey;
}

static const char *get_den_kw(rd_phase_enum phase, rd_version_enum rd_version) {
    if (rd_version == ECLIPSE100) {
        switch (phase) {
        case (RD_OIL_PHASE):
            return ECLIPSE100_OIL_DEN_KW;
            break;
        case (RD_GAS_PHASE):
            return ECLIPSE100_GAS_DEN_KW;
            break;
        case (RD_WATER_PHASE):
            return ECLIPSE100_WATER_DEN_KW;
            break;
        default:
            util_abort("%s: unrecognized phase id:%d \n", __func__, phase);
            return NULL;
        }
    } else if ((rd_version == ECLIPSE300) ||
               (rd_version == ECLIPSE300_THERMAL)) {
        switch (phase) {
        case (RD_OIL_PHASE):
            return ECLIPSE300_OIL_DEN_KW;
            break;
        case (RD_GAS_PHASE):
            return ECLIPSE300_GAS_DEN_KW;
            break;
        case (RD_WATER_PHASE):
            return ECLIPSE300_WATER_DEN_KW;
            break;
        default:
            util_abort("%s: unrecognized phase id:%d \n", __func__, phase);
            return NULL;
        }
    } else {
        util_abort("%s: unrecognized simulator id:%d \n", __func__, rd_version);
        return NULL;
    }
}

static void rd_grav_phase_ensure_work(rd_grav_phase_type *grav_phase) {
    if (grav_phase->work == NULL)
        grav_phase->work = (double *)util_calloc(grav_phase->grid_cache->size(),
                                                 sizeof *grav_phase->work);
}

static double rd_grav_phase_eval(rd_grav_phase_type *base_phase,
                                 const rd_grav_phase_type *monitor_phase,
                                 rd_region_type *region, double utm_x,
                                 double utm_y, double depth) {

    rd_grav_phase_ensure_work(base_phase);
    if ((monitor_phase == NULL) ||
        (base_phase->phase == monitor_phase->phase)) {
        const rd::rd_grid_cache &grid_cache = *(base_phase->grid_cache);
        const bool *aquifer = base_phase->aquifer_cell;
        double *mass_diff = base_phase->work;
        double deltag;
        /*
       Initialize a work array to contain the difference in mass for
       every cell.
    */
        {
            int index;
            if (monitor_phase == NULL) {
                for (index = 0; index < grid_cache.size(); index++)
                    mass_diff[index] = -base_phase->fluid_mass[index];
            } else {
                for (index = 0; index < grid_cache.size(); index++)
                    mass_diff[index] = monitor_phase->fluid_mass[index] -
                                       base_phase->fluid_mass[index];
            }
        }

        /**
       The Gravitational constant is 6.67E-11 N (m/kg)^2, we
       return the result in microGal, i.e. we scale with 10^2 *
       10^6 => 6.67E-3.
    */
        deltag = 6.67428E-3 * rd_grav_common_eval_biot_savart(
                                  grid_cache, region, aquifer, mass_diff, utm_x,
                                  utm_y, depth);

        return deltag;
    } else {
        util_abort("%s comparing different phases ... \n", __func__);
        return -1;
    }
}

static rd_grav_phase_type *
rd_grav_phase_alloc(rd_grav_type *rd_grav, rd_grav_survey_type *survey,
                    rd_phase_enum phase, const rd_file_view_type *restart_file,
                    grav_calc_type calc_type) {

    const rd_file_type *init_file = rd_grav->init_file;
    const rd::rd_grid_cache *grid_cache = rd_grav->grid_cache;
    const char *sat_kw_name = rd_get_phase_name(phase);
    {
        rd_grav_phase_type *grav_phase = new rd_grav_phase_type();
        const int size = grid_cache->size();

        UTIL_TYPE_ID_INIT(grav_phase, RD_GRAV_PHASE_TYPE_ID);
        grav_phase->grid_cache = grid_cache;
        grav_phase->aquifer_cell = rd_grav->aquifer_cell;
        grav_phase->fluid_mass =
            (double *)util_calloc(size, sizeof *grav_phase->fluid_mass);
        grav_phase->phase = phase;
        grav_phase->work = NULL;

        if (calc_type == GRAV_CALC_FIP) {
            rd_kw_type *pvtnum_kw =
                rd_file_iget_named_kw(init_file, PVTNUM_KW, 0);
            const std::vector<double> std_density =
                rd_grav->std_density[std::string(rd_get_phase_name(phase))];

            rd_kw_type *fip_kw;

            if (phase == RD_OIL_PHASE)
                fip_kw = rd_file_view_iget_named_kw(restart_file, FIPOIL_KW, 0);
            else if (phase == RD_GAS_PHASE)
                fip_kw = rd_file_view_iget_named_kw(restart_file, FIPGAS_KW, 0);
            else
                fip_kw = rd_file_view_iget_named_kw(restart_file, FIPWAT_KW, 0);

            for (int iactive = 0; iactive < size; iactive++) {
                double fip = rd_kw_iget_as_double(fip_kw, iactive);
                int pvtnum = rd_kw_iget_int(pvtnum_kw, iactive);
                if (std_density.size() <= pvtnum) {
                    rd_grav_phase_free(grav_phase);
                    return NULL;
                }
                grav_phase->fluid_mass[iactive] = fip * std_density[pvtnum];
            }
        } else {
            rd_version_enum rd_version = rd_file_get_rd_version(init_file);
            const char *den_kw_name = get_den_kw(phase, rd_version);
            const rd_kw_type *den_kw =
                rd_file_view_iget_named_kw(restart_file, den_kw_name, 0);

            if (calc_type == GRAV_CALC_RFIP) {
                rd_kw_type *rfip_kw;
                if (phase == RD_OIL_PHASE)
                    rfip_kw =
                        rd_file_view_iget_named_kw(restart_file, RFIPOIL_KW, 0);
                else if (phase == RD_GAS_PHASE)
                    rfip_kw =
                        rd_file_view_iget_named_kw(restart_file, RFIPGAS_KW, 0);
                else
                    rfip_kw =
                        rd_file_view_iget_named_kw(restart_file, RFIPWAT_KW, 0);

                {
                    int iactive;
                    for (iactive = 0; iactive < size; iactive++) {
                        double rho = rd_kw_iget_as_double(den_kw, iactive);
                        double rfip = rd_kw_iget_as_double(rfip_kw, iactive);
                        grav_phase->fluid_mass[iactive] = rho * rfip;
                    }
                }
            } else {
                /* (calc_type == GRAV_CALC_RPORV) || (calc_type == GRAV_CALC_PORMOD) */
                rd_kw_type *sat_kw;
                bool private_sat_kw = false;
                if (rd_file_view_has_kw(restart_file, sat_kw_name))
                    sat_kw = rd_file_view_iget_named_kw(restart_file,
                                                        sat_kw_name, 0);
                else {
                    /* We are targeting the residual phase, e.g. the OIL phase in a three phase system. */
                    const rd_kw_type *swat_kw =
                        rd_file_view_iget_named_kw(restart_file, "SWAT", 0);
                    sat_kw = rd_kw_alloc_copy(swat_kw);
                    rd_kw_scalar_set_float(sat_kw, 1.0);
                    rd_kw_inplace_sub(sat_kw, swat_kw); /* sat = 1 - SWAT */

                    if (rd_file_view_has_kw(restart_file, "SGAS")) {
                        const rd_kw_type *sgas_kw =
                            rd_file_view_iget_named_kw(restart_file, "SGAS", 0);
                        rd_kw_inplace_sub(sat_kw, sgas_kw); /* sat -= SGAS */
                    }
                    private_sat_kw = true;
                }

                {
                    int iactive;
                    for (iactive = 0; iactive < size; iactive++) {
                        double rho = rd_kw_iget_as_double(den_kw, iactive);
                        double sat = rd_kw_iget_as_double(sat_kw, iactive);
                        grav_phase->fluid_mass[iactive] =
                            rho * sat * survey->porv[iactive];
                    }
                }

                if (private_sat_kw)
                    rd_kw_free(sat_kw);
            }
        }

        return grav_phase;
    }
}

static void rd_grav_survey_add_phase(rd_grav_survey_type *survey,
                                     rd_phase_enum phase,
                                     rd_grav_phase_type *grav_phase) {
    survey->phase_list.push_back(grav_phase);
    survey->phase_map[std::string(rd_get_phase_name(phase))] = grav_phase;
}

static bool rd_grav_survey_add_phases(rd_grav_type *rd_grav,
                                      rd_grav_survey_type *survey,
                                      const rd_file_view_type *restart_file,
                                      grav_calc_type calc_type) {
    int phases = rd_file_get_phases(rd_grav->init_file);
    if (phases & RD_OIL_PHASE) {
        rd_grav_phase_type *oil_phase = rd_grav_phase_alloc(
            rd_grav, survey, RD_OIL_PHASE, restart_file, calc_type);
        if (oil_phase == NULL)
            return false;
        rd_grav_survey_add_phase(survey, RD_OIL_PHASE, oil_phase);
    }

    if (phases & RD_GAS_PHASE) {
        rd_grav_phase_type *gas_phase = rd_grav_phase_alloc(
            rd_grav, survey, RD_GAS_PHASE, restart_file, calc_type);
        if (gas_phase == NULL)
            return false;
        rd_grav_survey_add_phase(survey, RD_GAS_PHASE, gas_phase);
    }

    if (phases & RD_WATER_PHASE) {
        rd_grav_phase_type *water_phase = rd_grav_phase_alloc(
            rd_grav, survey, RD_WATER_PHASE, restart_file, calc_type);
        if (water_phase == NULL)
            return false;
        rd_grav_survey_add_phase(survey, RD_WATER_PHASE, water_phase);
    }
    return true;
}

static rd_grav_survey_type *
rd_grav_survey_alloc_empty(const rd_grav_type *rd_grav, const char *name,
                           grav_calc_type calc_type) {
    rd_grav_survey_type *survey = new rd_grav_survey_type();
    UTIL_TYPE_ID_INIT(survey, RD_GRAV_SURVEY_ID);
    survey->grid_cache = rd_grav->grid_cache;
    survey->aquifer_cell = rd_grav->aquifer_cell;
    survey->name = util_alloc_string_copy(name);

    if (calc_type & GRAV_CALC_USE_PORV)
        survey->porv = (double *)util_calloc(rd_grav->grid_cache->size(),
                                             sizeof *survey->porv);
    else
        survey->porv = NULL;

    return survey;
}

/**
   Check that the rporv values are in the right ballpark.  For ECLIPSE
   version 2008.2 they are way off. Check PORV versus RPORV
   for some random locations in the grid.
*/

static void rd_grav_survey_assert_RPORV(const rd_grav_survey_type *survey,
                                        const rd_file_type *init_file) {
    const rd::rd_grid_cache &grid_cache = *(survey->grid_cache);
    int active_size = grid_cache.size();
    const rd_kw_type *init_porv_kw =
        rd_file_iget_named_kw(init_file, PORV_KW, 0);
    int check_points = 100;
    int check_nr = 0;
    const std::vector<int> &global_index = grid_cache.global_index();

    while (check_nr < check_points) {
        int active_index = rand() % active_size;

        double init_porv = rd_kw_iget_as_double(
            init_porv_kw,
            global_index[active_index]); /* NB - this uses global indexing. */
        if (init_porv > 0) {
            double rporv = survey->porv[active_index];
            double log_pormod = log10(rporv / init_porv);

            if (fabs(log_pormod) > 1) {
                /* Detected as error if the effective pore volume multiplier
           is greater than 10 or less than 0.10. */
                fprintf(stderr, "----------------------------------------------"
                                "-------------------\n");
                fprintf(stderr, "INIT PORV : %g \n", init_porv);
                fprintf(stderr, "RPORV     : %g \n", rporv);
                fprintf(stderr, "Hmmm - the RPORV values extracted from the "
                                "restart file seem to be \n");
                fprintf(stderr, "veeery different from the initial porv value. "
                                "This might indicate \n");
                fprintf(stderr, "an ECLIPSE bug in the RPORV handling. Try "
                                "using another ECLIPSE version,\n");
                fprintf(stderr,
                        "or alternatively the PORMOD approach instead\n");
                fprintf(stderr, "----------------------------------------------"
                                "-------------------\n");
                exit(1);
            }
            check_nr++;
        }
    }
}

/**
   There are currently two main methods to add a survey; differentiated by
   how the mass of various phases in each cell is calculated:

    1. We can calculate the mass of each phase from the relation:

          mass = saturation * pore_volume * mass_density.

      This method requires access to the instantaneous pore volume. This
      can be accessed in two different ways, based either on the RPORV
      keyword or the PORV_MOD keyword. This functionality is available
      through the rd_grav_survey_alloc_RPORV() and
      rd_grav_survey_alloc_PORMOD() functions.


   2. The mass of each phase can be calculated based on the fluid in place
      values (volume of phase when the matter is brought to standard
      conditions), i.e. the FIPGAS, FIPWAT and FIPOIL keywords, and the
      corresponding densities at surface conditions. This functionality is
      implemented with the rd_grav_survey_alloc_FIP() function.

      Observe that use of the FIP based method requires densities entered
      with rd_grav_new_std_density()/rd_grav_add_std_density() prior to
      adding the actual survey.
*/

/**
   Allocate one survey based on using the RPORV keyword from the
   restart file to calculate the instantaneous pore volume in each
   cell.

   Unfortunately different versions of ECLIPSE have showed a wide
   range of bugs related to the RPORV keyword, including:

    - Using the pressure values instead of pore volumes - this will be
      cached by the rd_grav_survey_assert_RPORV() function.

    - Ignoring the dynamic pore volume changes, and just using
      RPORV  == INIT PORV.
*/

static rd_grav_survey_type *
rd_grav_survey_alloc_RPORV(rd_grav_type *rd_grav,
                           const rd_file_view_type *restart_file,
                           const char *name) {
    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_empty(rd_grav, name, GRAV_CALC_RPORV);

    if (rd_file_view_has_kw(restart_file, RPORV_KW)) {
        rd_kw_type *rporv_kw =
            rd_file_view_iget_named_kw(restart_file, RPORV_KW, 0);
        int iactive;
        for (iactive = 0; iactive < rd_kw_get_size(rporv_kw); iactive++)
            survey->porv[iactive] = rd_kw_iget_as_double(rporv_kw, iactive);
    } else
        util_abort("%s: restart file did not contain %s keyword??\n", __func__,
                   RPORV_KW);

    {
        const rd_file_type *init_file = rd_grav->init_file;
        rd_grav_survey_assert_RPORV(survey, init_file);
        if (!rd_grav_survey_add_phases(rd_grav, survey, restart_file,
                                       GRAV_CALC_RPORV)) {
            rd_grav_survey_free(survey);
            return NULL;
        }
    }
    return survey;
}

static rd_grav_survey_type *
rd_grav_survey_alloc_PORMOD(rd_grav_type *rd_grav,
                            const rd_file_view_type *restart_file,
                            const char *name) {
    rd::rd_grid_cache &grid_cache = *(rd_grav->grid_cache);
    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_empty(rd_grav, name, GRAV_CALC_PORMOD);

    rd_kw_type *init_porv_kw = rd_file_iget_named_kw(
        rd_grav->init_file, PORV_KW, 0); /* Global indexing */
    rd_kw_type *pormod_kw = rd_file_view_iget_named_kw(restart_file, PORMOD_KW,
                                                       0); /* Active indexing */
    const int size = grid_cache.size();
    const auto &global_index = grid_cache.global_index();
    int active_index;

    for (active_index = 0; active_index < size; active_index++)
        survey->porv[active_index] =
            rd_kw_iget_float(pormod_kw, active_index) *
            rd_kw_iget_float(init_porv_kw, global_index[active_index]);

    if (!rd_grav_survey_add_phases(rd_grav, survey, restart_file,
                                   GRAV_CALC_PORMOD)) {
        rd_grav_survey_free(survey);
        return NULL;
    }

    return survey;
}

/**
   Use of the rd_grav_survey_alloc_FIP() function requires that the densities
   have been added for all phases with the rd_grav_new_std_density() and
   possibly also the rd_grav_add_std_density() functions.
*/

static rd_grav_survey_type *
rd_grav_survey_alloc_FIP(rd_grav_type *rd_grav,
                         const rd_file_view_type *restart_file,
                         const char *name) {

    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_empty(rd_grav, name, GRAV_CALC_FIP);

    if (!rd_grav_survey_add_phases(rd_grav, survey, restart_file,
                                   GRAV_CALC_FIP)) {
        rd_grav_survey_free(survey);
        return NULL;
    }

    return survey;
}

static rd_grav_survey_type *
rd_grav_survey_alloc_RFIP(rd_grav_type *rd_grav,
                          const rd_file_view_type *restart_file,
                          const char *name) {

    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_empty(rd_grav, name, GRAV_CALC_RFIP);

    if (!rd_grav_survey_add_phases(rd_grav, survey, restart_file,
                                   GRAV_CALC_RFIP)) {
        rd_grav_survey_free(survey);
        return NULL;
    }

    return survey;
}

static double rd_grav_survey_eval(const rd_grav_survey_type *base_survey,
                                  const rd_grav_survey_type *monitor_survey,
                                  rd_region_type *region, double utm_x,
                                  double utm_y, double depth, int phase_mask) {
    double deltag = 0;
    for (std::size_t phase_nr = 0; phase_nr < base_survey->phase_list.size();
         phase_nr++) {
        rd_grav_phase_type *base_phase = base_survey->phase_list[phase_nr];
        if (base_phase->phase & phase_mask) {
            if (monitor_survey != NULL) {
                const rd_grav_phase_type *monitor_phase =
                    monitor_survey->phase_list[phase_nr];
                deltag += rd_grav_phase_eval(base_phase, monitor_phase, region,
                                             utm_x, utm_y, depth);
            } else
                deltag += rd_grav_phase_eval(base_phase, NULL, region, utm_x,
                                             utm_y, depth);
        }
    }
    return deltag;
}

/**
   The grid instance is only used during the construction phase. The
   @init_file object is used by the rd_grav_add_survey_XXX()
   functions; and calling scope must NOT destroy this object before
   all surveys have been added.
*/

rd_grav_type *rd_grav_alloc(const rd_grid_type *rd_grid,
                            const rd_file_type *init_file) {
    rd_grav_type *rd_grav = new rd_grav_type();

    rd_grav->init_file = init_file;
    rd_grav->grid_cache = new rd::rd_grid_cache(rd_grid);
    rd_grav->aquifer_cell = rd_grav_common_alloc_aquifer_cell(
        *(rd_grav->grid_cache), rd_grav->init_file);

    return rd_grav;
}

static void rd_grav_add_survey__(rd_grav_type *grav, const char *name,
                                 rd_grav_survey_type *survey) {
    grav->surveys[name] = survey;
}

rd_grav_survey_type *
rd_grav_add_survey_RPORV(rd_grav_type *grav, const char *name,
                         const rd_file_view_type *restart_file) {
    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_RPORV(grav, restart_file, name);
    if (survey == NULL)
        return NULL;
    rd_grav_add_survey__(grav, name, survey);
    return survey;
}

rd_grav_survey_type *
rd_grav_add_survey_FIP(rd_grav_type *grav, const char *name,
                       const rd_file_view_type *restart_file) {
    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_FIP(grav, restart_file, name);
    if (survey == NULL)
        return NULL;
    rd_grav_add_survey__(grav, name, survey);
    return survey;
}

rd_grav_survey_type *
rd_grav_add_survey_RFIP(rd_grav_type *grav, const char *name,
                        const rd_file_view_type *restart_file) {
    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_RFIP(grav, restart_file, name);
    if (survey == NULL)
        return NULL;
    rd_grav_add_survey__(grav, name, survey);
    return survey;
}

rd_grav_survey_type *
rd_grav_add_survey_PORMOD(rd_grav_type *grav, const char *name,
                          const rd_file_view_type *restart_file) {
    rd_grav_survey_type *survey =
        rd_grav_survey_alloc_PORMOD(grav, restart_file, name);
    if (survey == NULL)
        return NULL;
    rd_grav_add_survey__(grav, name, survey);
    return survey;
}

static rd_grav_survey_type *rd_grav_get_survey(const rd_grav_type *grav,
                                               const char *name) {
    if (name == NULL)
        return NULL; // Calling scope must determine if this is OK?
    else {
        if (grav->surveys.count(name) > 0)
            return grav->surveys.at(name);
        else {
            fprintf(stderr,
                    "Survey name:%s not registered. Available surveys are: "
                    "\n\n     ",
                    name);

            for (const auto &survey_pair : grav->surveys)
                fprintf(stderr, "%s ", survey_pair.first.c_str());

            fprintf(stderr, "\n\n");
            exit(1);
        }
    }
}

double rd_grav_eval(const rd_grav_type *grav, const char *base,
                    const char *monitor, rd_region_type *region, double utm_x,
                    double utm_y, double depth, int phase_mask) {
    rd_grav_survey_type *base_survey = rd_grav_get_survey(grav, base);
    rd_grav_survey_type *monitor_survey = rd_grav_get_survey(grav, monitor);

    return rd_grav_survey_eval(base_survey, monitor_survey, region, utm_x,
                               utm_y, depth, phase_mask);
}

/* The functions rd_grav_new_std_density() and rd_grav_add_std_density() are
   used to "install" standard conditions densities for the various phases
   involved. These functions must be called prior to calling
   rd_grav_add_survey_FIP() - failure to do so will lead to hard failure.
*/

/**
   The function rd_grav_new_std_density() is used to add a default density for
   a new phase.
*/

void rd_grav_new_std_density(rd_grav_type *grav, rd_phase_enum phase,
                             double default_density) {
    const char *phase_key = rd_get_phase_name(phase);
    grav->default_density[std::string(phase_key)] = default_density;
}

/**
   In cases with many PVT regions it is possible to install per PVT
   region densities. The rd_grav_new_std_density() must be called
   first to install a default density for the phase, and then this
   function can be called afterwards to install density for a
   particular PVT region.  In the example below we set the default gas
   density to 0.75, but in PVT regions 2 and 7 the density is
   different:

      rd_grav_new_std_density( grav , RD_GAS_PHASE , 0.75 );
      rd_grav_add_std_density( grav , RD_GAS_PHASE , 2 , 0.70 );
      rd_grav_add_std_density( grav , RD_GAS_PHASE , 7 , 0.80 );
*/

void rd_grav_add_std_density(rd_grav_type *grav, rd_phase_enum phase,
                             int pvtnum, double density) {
    std::vector<double> &std_density =
        grav->std_density[std::string(rd_get_phase_name(phase))];
    if (std_density.size() <= static_cast<std::size_t>(pvtnum))
        std_density.resize(
            pvtnum + 1,
            grav->default_density[std::string(rd_get_phase_name(phase))]);
    std_density[pvtnum] = density;
}

void rd_grav_free(rd_grav_type *rd_grav) {
    delete rd_grav->grid_cache;
    free(rd_grav->aquifer_cell);

    for (const auto &survey_pair : rd_grav->surveys)
        rd_grav_survey_free(survey_pair.second);

    delete rd_grav;
}
