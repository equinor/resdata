#include <time.h>
#include <math.h>

#include <vector>

#include <ert/util/util.h>
#include <ert/util/type_macros.hpp>

#include <resdata/rd_sum_tstep.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_type.hpp>

#define RD_SUM_TSTEP_ID 88631

/*
  This file implements the rd_sum_tstep datatype which contains the
  summary information for all summary vectors at one instant in
  time. If we view the summary data as this:


Header direction: rd_smspec   DAYS     WWCT:OP_3     FOPT     BPR:15,10,25
                               --------------------------------------------
  /|\                           0.00    0.00          0.00           256.00   <-- One timestep rd_sum_tstep
   |                           10.00    0.56         10.00           255.00
 Time direction: rd_sum_data  20.00    0.61         18.70           253.00
   |                           30.00    0.63         21.20           251.00
   |                           ...
  \|/                          90.00    0.80         39.70           244.00
                               --------------------------------------------

  The rd_sum_tstep structure corresponds to one 'horizontal line' in
  the summary data.

  These timesteps correspond exactly to the simulators timesteps,
  i.e. when convergence is poor they are closely spaced. In the
  summary files these time steps are called "MINISTEPS" - and
  that term is also used some places in the rd_sum_xxx codebase.
 */

struct rd_sum_tstep_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::vector<float>
        data; /* A memcpy copy of the PARAMS vector in rd_kw instance - the raw data. */
    time_t
        sim_time; /* The true time (i.e. 20.th of october 2010) of corresponding to this timestep. */
    int ministep; /* The simulator time-step number; one ministep per numerical timestep. */
    int report_step; /* The report step this time-step is part of - in general there can be many timestep for each report step. */
    double sim_seconds; /* Accumulated simulation time up to this ministep. */
    int internal_index; /* Used for lookups of the next / previous ministep based on an existing ministep. */
    const rd_smspec_type *
        smspec; /* The smespec header information for this tstep - must be compatible. */
};

rd_sum_tstep_type *
rd_sum_tstep_alloc_remap_copy(const rd_sum_tstep_type *src,
                              const rd_smspec_type *new_smspec,
                              float default_value, const int *params_map) {
    int params_size = rd_smspec_get_params_size(new_smspec);
    rd_sum_tstep_type *target = new rd_sum_tstep_type();
    UTIL_TYPE_ID_INIT(target, RD_SUM_TSTEP_ID);
    target->report_step = src->report_step;
    target->ministep = src->ministep;

    target->smspec = new_smspec;
    target->data.resize(params_size);
    for (int i = 0; i < params_size; i++) {

        if (params_map[i] >= 0)
            target->data[i] = src->data[params_map[i]];
        else
            target->data[i] = default_value;
    }
    return target;
}

rd_sum_tstep_type *rd_sum_tstep_alloc_copy(const rd_sum_tstep_type *src) {
    rd_sum_tstep_type *target = new rd_sum_tstep_type();
    UTIL_TYPE_ID_INIT(target, RD_SUM_TSTEP_ID);
    target->smspec = src->smspec;
    target->report_step = src->report_step;
    target->ministep = src->ministep;
    target->data = src->data;
    return target;
}

static rd_sum_tstep_type *rd_sum_tstep_alloc(int report_step, int ministep_nr,
                                             const rd_smspec_type *smspec) {
    rd_sum_tstep_type *tstep = new rd_sum_tstep_type();
    UTIL_TYPE_ID_INIT(tstep, RD_SUM_TSTEP_ID);
    tstep->smspec = smspec;
    tstep->report_step = report_step;
    tstep->ministep = ministep_nr;
    tstep->data.resize(rd_smspec_get_params_size(smspec));
    return tstep;
}

UTIL_SAFE_CAST_FUNCTION(rd_sum_tstep, RD_SUM_TSTEP_ID)
UTIL_SAFE_CAST_FUNCTION_CONST(rd_sum_tstep, RD_SUM_TSTEP_ID)

void rd_sum_tstep_free(rd_sum_tstep_type *ministep) { delete ministep; }

void rd_sum_tstep_free__(void *__ministep) {
    rd_sum_tstep_type *ministep = rd_sum_tstep_safe_cast(__ministep);
    rd_sum_tstep_free(ministep);
}

/**
   This function sets the internal time representation in the
   rd_sum_tstep. The treatment of time is a bit weird; on the one
   hand the time elements in the summary data are just like any other
   element like e.g. the FOPT or GGPR:NAME - on the other hand the
   time information is strictly required and the summary file will
   fall to pieces if it is missing.

   The time can be provided in using (at least) two different
   keywords:

      DAYS/HOURS: The data vector will contain the number of
            days/hours since the simulation start (hours in the case
            of lab units).

      DAY,MONTH,YEAR: The data vector will contain the true date of
           the tstep.

   The rd_sum_tstep class can utilize both types of information, but
   will select the DAYS variety if both are present.
*/

static void rd_sum_tstep_set_time_info_from_seconds(rd_sum_tstep_type *tstep,
                                                    time_t sim_start,
                                                    double sim_seconds) {
    tstep->sim_seconds = sim_seconds;
    tstep->sim_time = sim_start;
    util_inplace_forward_seconds_utc(&tstep->sim_time, tstep->sim_seconds);
}

static void rd_sum_tstep_set_time_info_from_date(rd_sum_tstep_type *tstep,
                                                 time_t sim_start,
                                                 time_t sim_time) {
    tstep->sim_time = sim_time;
    tstep->sim_seconds = util_difftime_seconds(sim_start, tstep->sim_time);
}

static void rd_sum_tstep_set_time_info(rd_sum_tstep_type *tstep,
                                       const rd_smspec_type *smspec) {
    int date_day_index = rd_smspec_get_date_day_index(smspec);
    int date_month_index = rd_smspec_get_date_month_index(smspec);
    int date_year_index = rd_smspec_get_date_year_index(smspec);
    int sim_time_index = rd_smspec_get_time_index(smspec);
    time_t sim_start = rd_smspec_get_start_time(smspec);

    if (sim_time_index >= 0) {
        double sim_time = tstep->data[sim_time_index];
        double sim_seconds = sim_time * rd_smspec_get_time_seconds(smspec);
        rd_sum_tstep_set_time_info_from_seconds(tstep, sim_start, sim_seconds);
    } else if (date_day_index >= 0) {
        int day = util_roundf(tstep->data[date_day_index]);
        int month = util_roundf(tstep->data[date_month_index]);
        int year = util_roundf(tstep->data[date_year_index]);

        time_t sim_time = rd_make_date(day, month, year);
        rd_sum_tstep_set_time_info_from_date(tstep, sim_start, sim_time);
    } else
        util_abort("%s: Hmmm - could not extract date/time information from "
                   "SMSPEC header file? \n",
                   __func__);
}

/**
   If the rd_kw instance is in some way invalid (i.e. wrong size);
   the function will return NULL:
*/

rd_sum_tstep_type *rd_sum_tstep_alloc_from_file(int report_step,
                                                int ministep_nr,
                                                const rd_kw_type *params_kw,
                                                const char *src_file,
                                                const rd_smspec_type *smspec) {

    int data_size = rd_kw_get_size(params_kw);

    if (data_size == rd_smspec_get_params_size(smspec)) {
        rd_sum_tstep_type *ministep =
            rd_sum_tstep_alloc(report_step, ministep_nr, smspec);
        rd_kw_get_memcpy_data(params_kw, ministep->data.data());
        rd_sum_tstep_set_time_info(ministep, smspec);
        return ministep;
    } else {
        /*
       This is actually a fatal error / bug; the difference in smspec
       header structure should have been detected already in the
       rd_smspec_load_restart() function and the restart case
       discarded.
    */
        fprintf(stderr,
                "** Warning size mismatch between timestep loaded from:%s(%d) "
                "and header:%s(%d) - timestep discarded.\n",
                src_file, data_size, rd_smspec_get_header_file(smspec),
                rd_smspec_get_params_size(smspec));
        return NULL;
    }
}

/*
  Should be called in write mode.
*/

rd_sum_tstep_type *rd_sum_tstep_alloc_new(int report_step, int ministep,
                                          float sim_seconds,
                                          const rd_smspec_type *smspec) {
    rd_sum_tstep_type *tstep =
        rd_sum_tstep_alloc(report_step, ministep, smspec);
    tstep->data = rd_smspec_get_params_default(smspec);

    rd_sum_tstep_set_time_info_from_seconds(
        tstep, rd_smspec_get_start_time(smspec), sim_seconds);
    rd_sum_tstep_iset(tstep, rd_smspec_get_time_index(smspec),
                      sim_seconds / rd_smspec_get_time_seconds(smspec));
    return tstep;
}

double rd_sum_tstep_iget(const rd_sum_tstep_type *ministep, int index) {
    if ((index >= 0) && (index < (int)ministep->data.size()))
        return ministep->data[index];
    else {
        util_abort("%s: param index:%d invalid: Valid range: [0,%d) \n",
                   __func__, index, ministep->data.size());
        return -1;
    }
}

time_t rd_sum_tstep_get_sim_time(const rd_sum_tstep_type *ministep) {
    return ministep->sim_time;
}

double rd_sum_tstep_get_sim_days(const rd_sum_tstep_type *ministep) {
    return ministep->sim_seconds / (24 * 3600);
}

double rd_sum_tstep_get_sim_seconds(const rd_sum_tstep_type *ministep) {
    return ministep->sim_seconds;
}

int rd_sum_tstep_get_report(const rd_sum_tstep_type *ministep) {
    return ministep->report_step;
}

int rd_sum_tstep_get_ministep(const rd_sum_tstep_type *ministep) {
    return ministep->ministep;
}

void rd_sum_tstep_fwrite(const rd_sum_tstep_type *ministep,
                         const int *index_map, int index_map_size,
                         fortio_type *fortio) {
    {
        rd_kw_type *ministep_kw = rd_kw_alloc(MINISTEP_KW, 1, RD_INT);
        rd_kw_iset_int(ministep_kw, 0, ministep->ministep);
        rd_kw_fwrite(ministep_kw, fortio);
        rd_kw_free(ministep_kw);
    }

    {
        int compact_size = index_map_size;
        rd_kw_type *params_kw = rd_kw_alloc(PARAMS_KW, compact_size, RD_FLOAT);

        float *data = (float *)rd_kw_get_ptr(params_kw);

        {
            int i;
            for (i = 0; i < compact_size; i++)
                data[i] = ministep->data[index_map[i]];
        }
        rd_kw_fwrite(params_kw, fortio);
        rd_kw_free(params_kw);
    }
}

void rd_sum_tstep_iset(rd_sum_tstep_type *tstep, int index, float value) {
    if ((index < static_cast<int>(tstep->data.size())) && (index >= 0))
        tstep->data[index] = value;
    else
        util_abort("%s: index:%d invalid. Valid range: [0,%d) \n", __func__,
                   index, tstep->data.size());
}

void rd_sum_tstep_iscale(rd_sum_tstep_type *tstep, int index, float scalar) {
    rd_sum_tstep_iset(tstep, index, rd_sum_tstep_iget(tstep, index) * scalar);
}

void rd_sum_tstep_ishift(rd_sum_tstep_type *tstep, int index, float addend) {
    rd_sum_tstep_iset(tstep, index, rd_sum_tstep_iget(tstep, index) + addend);
}

void rd_sum_tstep_set_from_node(rd_sum_tstep_type *tstep,
                                const rd::smspec_node &smspec_node,
                                float value) {
    int data_index = smspec_node_get_params_index(&smspec_node);
    rd_sum_tstep_iset(tstep, data_index, value);
}

double rd_sum_tstep_get_from_node(const rd_sum_tstep_type *tstep,
                                  const rd::smspec_node &smspec_node) {
    int data_index = smspec_node_get_params_index(&smspec_node);
    return rd_sum_tstep_iget(tstep, data_index);
}

void rd_sum_tstep_set_from_key(rd_sum_tstep_type *tstep, const char *gen_key,
                               float value) {
    const rd::smspec_node &smspec_node =
        rd_smspec_get_general_var_node(tstep->smspec, gen_key);
    rd_sum_tstep_set_from_node(tstep, smspec_node, value);
}

double rd_sum_tstep_get_from_key(const rd_sum_tstep_type *tstep,
                                 const char *gen_key) {
    const rd::smspec_node &smspec_node =
        rd_smspec_get_general_var_node(tstep->smspec, gen_key);
    return rd_sum_tstep_get_from_node(tstep, smspec_node);
}

bool rd_sum_tstep_has_key(const rd_sum_tstep_type *tstep, const char *gen_key) {
    return rd_smspec_has_general_var(tstep->smspec, gen_key);
}

bool rd_sum_tstep_sim_time_equal(const rd_sum_tstep_type *tstep1,
                                 const rd_sum_tstep_type *tstep2) {
    if (tstep1->sim_time == tstep2->sim_time)
        return true;
    else
        return false;
}
