#ifndef ERT_RD_SUM_DATA_H
#define ERT_RD_SUM_DATA_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <ert/util/time_t_vector.hpp>
#include <ert/util/double_vector.hpp>
#include <ert/util/stringlist.hpp>

#include <resdata/rd_sum_tstep.hpp>
#include <resdata/smspec_node.hpp>
#include <resdata/rd_sum_vector.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_sum_data_struct rd_sum_data_type;

void rd_sum_data_reset_self_map(rd_sum_data_type *data);
void rd_sum_data_add_case(rd_sum_data_type *self,
                          const rd_sum_data_type *other);
void rd_sum_data_fwrite_step(const rd_sum_data_type *data, const char *rd_case,
                             bool fmt_case, bool unified, int report_step);
void rd_sum_data_fwrite(const rd_sum_data_type *data, const char *rd_case,
                        bool fmt_case, bool unified);
bool rd_sum_data_can_write(const rd_sum_data_type *data);
bool rd_sum_data_fread(rd_sum_data_type *data, const stringlist_type *filelist,
                       bool lazy_load, int file_options);
rd_sum_data_type *rd_sum_data_alloc_writer(rd_smspec_type *smspec);
rd_sum_data_type *rd_sum_data_alloc(rd_smspec_type *smspec);
double rd_sum_data_time2days(const rd_sum_data_type *data, time_t sim_time);
int rd_sum_data_get_report_step_from_time(const rd_sum_data_type *data,
                                          time_t sim_time);
int rd_sum_data_get_report_step_from_days(const rd_sum_data_type *data,
                                          double days);
bool rd_sum_data_check_sim_time(const rd_sum_data_type *data, time_t sim_time);
bool rd_sum_data_check_sim_days(const rd_sum_data_type *data, double sim_days);
int rd_sum_data_get_num_ministep(const rd_sum_data_type *data);
double_vector_type *rd_sum_data_alloc_data_vector(const rd_sum_data_type *data,
                                                  int data_index,
                                                  bool report_only);
void rd_sum_data_init_time_vector(const rd_sum_data_type *data,
                                  time_t_vector_type *time_vector,
                                  bool report_only);
time_t_vector_type *rd_sum_data_alloc_time_vector(const rd_sum_data_type *data,
                                                  bool report_only);
time_t rd_sum_data_get_data_start(const rd_sum_data_type *data);
time_t rd_sum_data_get_report_time(const rd_sum_data_type *data,
                                   int report_step);
double rd_sum_data_get_first_day(const rd_sum_data_type *data);
time_t rd_sum_data_get_sim_start(const rd_sum_data_type *data);
time_t rd_sum_data_get_sim_end(const rd_sum_data_type *data);
double rd_sum_data_get_sim_length(const rd_sum_data_type *data);
void rd_sum_data_summarize(const rd_sum_data_type *data, FILE *stream);
double rd_sum_data_iget(const rd_sum_data_type *data, int internal_index,
                        int params_index);

double rd_sum_data_iget_sim_days(const rd_sum_data_type *, int);
time_t rd_sum_data_iget_sim_time(const rd_sum_data_type *, int);
void rd_sum_data_get_interp_vector(const rd_sum_data_type *data,
                                   time_t sim_time,
                                   const rd_sum_vector_type *keylist,
                                   double_vector_type *results);

bool rd_sum_data_has_report_step(const rd_sum_data_type *, int);

rd_sum_data_type *rd_sum_data_fread_alloc(rd_smspec_type *,
                                          const stringlist_type *filelist,
                                          bool include_restart, bool lazy_load);
void rd_sum_data_free(rd_sum_data_type *);
int rd_sum_data_get_last_report_step(const rd_sum_data_type *data);
int rd_sum_data_get_first_report_step(const rd_sum_data_type *data);

double rd_sum_data_get_from_sim_time(const rd_sum_data_type *data,
                                     time_t sim_time,
                                     const rd::smspec_node &smspec_node);
double rd_sum_data_get_from_sim_days(const rd_sum_data_type *data,
                                     double sim_days,
                                     const rd::smspec_node &smspec_node);

int rd_sum_data_get_length(const rd_sum_data_type *data);
int rd_sum_data_iget_report_step(const rd_sum_data_type *data,
                                 int internal_index);
int rd_sum_data_iget_report_end(const rd_sum_data_type *data, int report_step);
rd_sum_tstep_type *rd_sum_data_add_new_tstep(rd_sum_data_type *data,
                                             int report_step,
                                             double sim_seconds);
bool rd_sum_data_report_step_equal(const rd_sum_data_type *data1,
                                   const rd_sum_data_type *data2);
bool rd_sum_data_report_step_compatible(const rd_sum_data_type *data1,
                                        const rd_sum_data_type *data2);
void rd_sum_data_fwrite_interp_csv_line(const rd_sum_data_type *data,
                                        time_t sim_time,
                                        const rd_sum_vector_type *keylist,
                                        FILE *fp);
double rd_sum_data_get_last_value(const rd_sum_data_type *data,
                                  int param_index);
double rd_sum_data_iget_last_value(const rd_sum_data_type *data,
                                   int param_index);
double rd_sum_data_iget_first_value(const rd_sum_data_type *data,
                                    int param_index);
void rd_sum_data_init_double_vector(const rd_sum_data_type *data,
                                    int params_index, double *output_data);
void rd_sum_data_init_datetime64_vector(const rd_sum_data_type *data,
                                        int64_t *output_data, int multiplier);

void rd_sum_data_init_double_frame(const rd_sum_data_type *data,
                                   const rd_sum_vector_type *keywords,
                                   double *output_data);
double_vector_type *
rd_sum_data_alloc_seconds_solution(const rd_sum_data_type *data,
                                   const rd::smspec_node &node, double value,
                                   bool rates_clamp_lower);
void rd_sum_data_init_double_frame_interp(const rd_sum_data_type *data,
                                          const rd_sum_vector_type *keywords,
                                          const time_t_vector_type *time_points,
                                          double *output_data);

void rd_sum_data_init_double_vector_interp(
    const rd_sum_data_type *data, const rd::smspec_node &smspec_node,
    const time_t_vector_type *time_points, double *output_data);

#ifdef __cplusplus
}
#endif
#endif
