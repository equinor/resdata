#ifndef ERT_RD_SUM_H
#define ERT_RD_SUM_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/double_vector.hpp>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/smspec_node.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *locale;
    const char *sep;
    const char *newline;
    const char *value_fmt;
    const char *date_fmt;
    const char *days_fmt;
    const char *header_fmt;
    bool print_header;
    bool print_dash;
    const char *date_header;
    const char *date_dash;
    const char *value_dash;
} rd_sum_fmt_type;

typedef struct rd_sum_vector_struct rd_sum_vector_type;

typedef struct rd_sum_struct rd_sum_type;

void rd_sum_fmt_init_summary_x(const rd_sum_type *rd_sum, rd_sum_fmt_type *fmt);
double rd_sum_get_from_sim_time(const rd_sum_type *rd_sum, time_t sim_time,
                                const rd::smspec_node *node);
double rd_sum_time2days(const rd_sum_type *rd_sum, time_t sim_time);

int rd_sum_get_report_step_from_time(const rd_sum_type *sum, time_t sim_time);
int rd_sum_get_report_step_from_days(const rd_sum_type *sum, double sim_days);
bool rd_sum_check_sim_time(const rd_sum_type *sum, time_t sim_time);
bool rd_sum_check_sim_days(const rd_sum_type *sum, double sim_days);
const char *rd_sum_get_unit(const rd_sum_type *sum, const char *gen_key);

double rd_sum_iget(const rd_sum_type *rd_sum, int time_index, int param_index);
int rd_sum_get_data_length(const rd_sum_type *rd_sum);

void rd_sum_free_data(rd_sum_type *);
void rd_sum_free(rd_sum_type *);
rd_sum_type *rd_sum_fread_alloc(const char *, const stringlist_type *data_files,
                                const char *key_join_string,
                                bool include_restart, bool lazy_load,
                                int file_options);
rd_sum_type *rd_sum_fread_alloc_case(const char *, const char *key_join_string);
rd_sum_type *rd_sum_fread_alloc_case2__(const char *,
                                        const char *key_join_string,
                                        bool include_restart, bool lazy_load,
                                        int file_options);
rd_sum_type *rd_sum_alloc_resample(const rd_sum_type *rd_sum,
                                   const char *rd_case,
                                   const time_t_vector_type *times,
                                   bool lower_extrapolation,
                                   bool upper_extrapolation);

double rd_sum_get_well_completion_var(const rd_sum_type *rd_sum, int time_index,
                                      const char *well, const char *var,
                                      int cell_nr);

double rd_sum_get_general_var(const rd_sum_type *rd_sum, int time_index,
                              const char *lookup_kw);
int rd_sum_get_general_var_params_index(const rd_sum_type *rd_sum,
                                        const char *lookup_kw);
const rd::smspec_node *rd_sum_get_general_var_node(const rd_sum_type *rd_sum,
                                                   const char *lookup_kw);
bool rd_sum_has_general_var(const rd_sum_type *rd_sum, const char *lookup_kw);
bool rd_sum_has_key(const rd_sum_type *rd_sum, const char *lookup_kw);
double rd_sum_get_general_var_from_sim_days(const rd_sum_type *rd_sum,
                                            double sim_days, const char *var);
double rd_sum_get_general_var_from_sim_time(const rd_sum_type *rd_sum,
                                            time_t sim_time, const char *var);
ert_rd_unit_enum rd_sum_get_unit_system(const rd_sum_type *rd_sum);

void rd_sum_fprintf(const rd_sum_type *, FILE *, const stringlist_type *,
                    bool report_only, const rd_sum_fmt_type *fmt);

int rd_sum_get_restart_step(const rd_sum_type *rd_sum);
int rd_sum_get_first_gt(const rd_sum_type *rd_sum, int param_index,
                        double limit);
int rd_sum_get_first_lt(const rd_sum_type *rd_sum, int param_index,
                        double limit);
int rd_sum_get_last_report_step(const rd_sum_type *rd_sum);
int rd_sum_get_first_report_step(const rd_sum_type *rd_sum);
time_t rd_sum_get_report_time(const rd_sum_type *rd_sum, int report_step);
time_t rd_sum_iget_sim_time(const rd_sum_type *rd_sum, int index);
double rd_sum_iget_sim_days(const rd_sum_type *rd_sum, int time_index);
int rd_sum_iget_report_step(const rd_sum_type *rd_sum, int internal_index);

double_vector_type *rd_sum_alloc_data_vector(const rd_sum_type *rd_sum,
                                             int data_index, bool report_only);
time_t_vector_type *rd_sum_alloc_time_vector(const rd_sum_type *rd_sum,
                                             bool report_only);
time_t rd_sum_get_data_start(const rd_sum_type *rd_sum);
time_t rd_sum_get_end_time(const rd_sum_type *rd_sum);
time_t rd_sum_get_start_time(const rd_sum_type *);

const char *rd_sum_get_base(const rd_sum_type *rd_sum);
const char *rd_sum_get_path(const rd_sum_type *rd_sum);
const char *rd_sum_get_abs_path(const rd_sum_type *rd_sum);
const rd_sum_type *rd_sum_get_restart_case(const rd_sum_type *rd_sum);
const char *rd_sum_get_case(const rd_sum_type *);

double rd_sum_get_sim_length(const rd_sum_type *rd_sum);
double rd_sum_get_first_day(const rd_sum_type *rd_sum);

stringlist_type *rd_sum_alloc_well_list(const rd_sum_type *rd_sum,
                                        const char *pattern);
stringlist_type *rd_sum_alloc_group_list(const rd_sum_type *rd_sum,
                                         const char *pattern);
stringlist_type *
rd_sum_alloc_matching_general_var_list(const rd_sum_type *rd_sum,
                                       const char *pattern);
void rd_sum_select_matching_general_var_list(const rd_sum_type *rd_sum,
                                             const char *pattern,
                                             stringlist_type *keys);
rd_smspec_type *rd_sum_get_smspec(const rd_sum_type *rd_sum);
rd_smspec_var_type rd_sum_identify_var_type(const char *var);

int rd_sum_iget_report_end(const rd_sum_type *rd_sum, int report_step);
rd_sum_type *
rd_sum_alloc_restart_writer2(const char *rd_case, const char *restart_case,
                             int restart_step, bool fmt_output, bool unified,
                             const char *key_join_string, time_t sim_start,
                             bool time_in_days, int nx, int ny, int nz);
void rd_sum_set_case(rd_sum_type *rd_sum, const char *input_arg);

rd_sum_type *rd_sum_alloc_restart_writer(const char *rd_case,
                                         const char *restart_case,
                                         bool fmt_output, bool unified,
                                         const char *key_join_string,
                                         time_t sim_start, bool time_in_days,
                                         int nx, int ny, int nz);
rd_sum_type *rd_sum_alloc_writer(const char *rd_case, bool fmt_output,
                                 bool unified, const char *key_join_string,
                                 time_t sim_start, bool time_in_days, int nx,
                                 int ny, int nz);
void rd_sum_fwrite(const rd_sum_type *rd_sum);
bool rd_sum_can_write(const rd_sum_type *rd_sum);
const rd::smspec_node *rd_sum_add_smspec_node(rd_sum_type *rd_sum,
                                              const rd::smspec_node *node);
const rd::smspec_node *rd_sum_add_var(rd_sum_type *rd_sum, const char *keyword,
                                      const char *wgname, int num,
                                      const char *unit, float default_value);
const rd::smspec_node *rd_sum_add_local_var(rd_sum_type *rd_sum,
                                            const char *keyword,
                                            const char *wgname, int num,
                                            const char *unit, const char *lgr,
                                            int lgr_i, int lgr_j, int lgr_k,
                                            float default_value);
rd_sum_tstep_type *rd_sum_add_tstep(rd_sum_type *rd_sum, int report_step,
                                    double sim_seconds);

void rd_sum_export_csv(const rd_sum_type *rd_sum, const char *filename,
                       const stringlist_type *var_list, const char *date_format,
                       const char *sep);

double_vector_type *rd_sum_alloc_days_solution(const rd_sum_type *rd_sum,
                                               const char *gen_key,
                                               double cmp_value,
                                               bool rates_clamp_lower);
time_t_vector_type *rd_sum_alloc_time_solution(const rd_sum_type *rd_sum,
                                               const char *gen_key,
                                               double cmp_value,
                                               bool rates_clamp_lower);

double rd_sum_get_last_value_gen_key(const rd_sum_type *rd_sum,
                                     const char *gen_key);
double rd_sum_get_first_value_gen_key(const rd_sum_type *rd_sum,
                                      const char *gen_key);
void rd_sum_init_datetime64_vector(const rd_sum_type *rd_sum, int64_t *data,
                                   int multiplier);
void rd_sum_init_double_vector_interp(const rd_sum_type *rd_sum,
                                      const char *gen_key,
                                      const time_t_vector_type *time_points,
                                      double *data);
void rd_sum_init_double_vector(const rd_sum_type *rd_sum, const char *gen_key,
                               double *data);
void rd_sum_init_double_frame(const rd_sum_type *rd_sum,
                              const rd_sum_vector_type *keywords, double *data);
void rd_sum_init_double_frame_interp(const rd_sum_type *rd_sum,
                                     const rd_sum_vector_type *keywords,
                                     const time_t_vector_type *time_points,
                                     double *data);
UTIL_IS_INSTANCE_HEADER(rd_sum);

#ifdef __cplusplus
}
#endif
#endif
