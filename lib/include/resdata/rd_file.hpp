#ifndef ERT_RD_FILE_H
#define ERT_RD_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>

#define RD_FILE_FLAGS_ENUM_DEFS                                                \
    {.value = 1, .name = "RD_FILE_CLOSE_STREAM"}, {                            \
        .value = 2, .name = "RD_FILE_WRITABLE"                                 \
    }
#define RD_FILE_FLAGS_ENUM_SIZE 2

typedef struct rd_file_struct rd_file_type;
bool rd_file_load_all(rd_file_type *rd_file);
rd_file_type *rd_file_open(const char *filename, int flags);
rd_file_type *rd_file_fast_open(const char *filename,
                                const char *index_filename, int flags);
bool rd_file_write_index(const rd_file_type *rd_file,
                         const char *index_filename);
bool rd_file_index_valid(const char *file_name, const char *index_file_name);
void rd_file_close(rd_file_type *rd_file);
void rd_file_fortio_detach(rd_file_type *rd_file);
void rd_file_free__(void *arg);
rd_kw_type *rd_file_icopy_named_kw(const rd_file_type *rd_file, const char *kw,
                                   int ith);
rd_kw_type *rd_file_icopy_kw(const rd_file_type *rd_file, int index);
bool rd_file_has_kw(const rd_file_type *rd_file, const char *kw);
int rd_file_get_num_named_kw(const rd_file_type *rd_file, const char *kw);
int rd_file_get_size(const rd_file_type *rd_file);
int rd_file_get_num_distinct_kw(const rd_file_type *rd_file);
const char *rd_file_iget_distinct_kw(const rd_file_type *rd_file, int index);
const char *rd_file_get_src_file(const rd_file_type *rd_file);
int rd_file_iget_occurence(const rd_file_type *rd_file, int index);
rd_version_enum rd_file_get_rd_version(const rd_file_type *file);
void rd_file_fwrite_fortio(const rd_file_type *ec_file, fortio_type *fortio,
                           int offset);
void rd_file_fwrite(const rd_file_type *rd_file, const char *, bool fmt_file);

void rd_file_replace_kw(rd_file_type *rd_file, rd_kw_type *old_kw,
                        rd_kw_type *new_kw, bool insert_copy);
int rd_file_get_phases(const rd_file_type *init_file);
void rd_file_fprintf_kw_list(const rd_file_type *rd_file, FILE *stream);

bool rd_file_writable(const rd_file_type *rd_file);
int rd_file_get_flags(const rd_file_type *rd_file);
void rd_file_set_flags(rd_file_type *rd_file, int new_flags);
bool rd_file_flags_set(const rd_file_type *rd_file, int flags);

rd_file_kw_type *rd_file_iget_file_kw(const rd_file_type *file,
                                      int global_index);
rd_file_kw_type *rd_file_iget_named_file_kw(const rd_file_type *file,
                                            const char *kw, int ith);
rd_kw_type *rd_file_iget_kw(const rd_file_type *file, int global_index);
rd_data_type rd_file_iget_data_type(const rd_file_type *file, int global_index);
int rd_file_iget_size(const rd_file_type *file, int global_index);
const char *rd_file_iget_header(const rd_file_type *file, int global_index);
rd_kw_type *rd_file_iget_named_kw(const rd_file_type *file, const char *kw,
                                  int ith);
rd_data_type rd_file_iget_named_data_type(const rd_file_type *file,
                                          const char *kw, int ith);
int rd_file_iget_named_size(const rd_file_type *file, const char *kw, int ith);
void rd_file_indexed_read(const rd_file_type *file, const char *kw, int index,
                          const int_vector_type *index_map, char *buffer);

rd_file_view_type *rd_file_get_global_blockview(rd_file_type *rd_file,
                                                const char *kw, int occurence);
rd_file_view_type *rd_file_alloc_global_blockview(rd_file_type *rd_file,
                                                  const char *kw,
                                                  int occurence);
rd_file_view_type *rd_file_get_global_view(rd_file_type *rd_file);
rd_file_view_type *rd_file_get_active_view(rd_file_type *rd_file);
//bool               rd_file_writable( const rd_file_type * rd_file );
bool rd_file_save_kw(const rd_file_type *rd_file, const rd_kw_type *rd_kw);
bool rd_file_has_kw_ptr(const rd_file_type *rd_file, const rd_kw_type *rd_kw);

double rd_file_iget_restart_sim_days(const rd_file_type *restart_file,
                                     int index);
time_t rd_file_iget_restart_sim_date(const rd_file_type *restart_file,
                                     int occurence);
int rd_file_get_restart_index(const rd_file_type *restart_file,
                              time_t sim_time);
bool rd_file_has_report_step(const rd_file_type *rd_file, int report_step);
bool rd_file_has_sim_time(const rd_file_type *rd_file, time_t sim_time);

void rd_file_close_fortio_stream(rd_file_type *rd_file);

rd_file_view_type *rd_file_get_restart_view(rd_file_type *rd_file,
                                            int input_index, int report_step,
                                            time_t sim_time, double sim_days);
rd_file_view_type *rd_file_get_summary_view(rd_file_type *rd_file,
                                            int report_step);

UTIL_IS_INSTANCE_HEADER(rd_file);

//Deprecated:

void rd_file_push_block(rd_file_type *rd_file);
void rd_file_pop_block(rd_file_type *rd_file);
bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence);
bool rd_file_select_block(rd_file_type *rd_file, const char *kw, int occurence);

bool rd_file_select_rstblock_sim_time(rd_file_type *rd_file, time_t sim_time);
bool rd_file_select_rstblock_report_step(rd_file_type *rd_file,
                                         int report_step);
bool rd_file_iselect_rstblock(rd_file_type *rd_file, int seqnum_index);
rd_file_type *rd_file_open_rstblock_report_step(const char *filename,
                                                int report_step, int flags);
rd_file_type *rd_file_open_rstblock_sim_time(const char *filename,
                                             time_t sim_time, int flags);
rd_file_type *rd_file_iopen_rstblock(const char *filename, int seqnum_index,
                                     int flags);

#ifdef __cplusplus
}
#endif
#endif
