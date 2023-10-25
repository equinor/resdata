#ifndef ERT_RD_FILE_VIEW_H
#define ERT_RD_FILE_VIEW_H

#include <stdlib.h>
#include <stdbool.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_type.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RD_FILE_CLOSE_STREAM = 1, /*
                                    This flag will close the underlying FILE object between each access; this is
                                    mainly to save filedescriptors in cases where many rd_file instances are open at
                                    the same time. */
    RD_FILE_WRITABLE = 2      /*
                                    This flag opens the file in a mode where it can be updated and modified, but it
                                    must still exist and be readable. I.e. this should not compared with the normal:
                                    fopen(filename , "w") where an existing file is truncated to zero upon successfull
                                    open.
                                 */
} rd_file_flag_type;

typedef struct rd_file_view_struct rd_file_view_type;
typedef struct rd_file_transaction_struct rd_file_transaction_type;

bool rd_file_view_flags_set(const rd_file_view_type *file_view,
                            int query_flags);
bool rd_file_view_check_flags(int state_flags, int query_flags);

rd_file_view_type *rd_file_view_alloc(fortio_type *fortio, int *flags,
                                      inv_map_type *inv_map, bool owner);
int rd_file_view_get_global_index(const rd_file_view_type *rd_file_view,
                                  const char *kw, int ith);
void rd_file_view_make_index(rd_file_view_type *rd_file_view);
bool rd_file_view_has_kw(const rd_file_view_type *rd_file_view, const char *kw);
rd_file_kw_type *
rd_file_view_iget_file_kw(const rd_file_view_type *rd_file_view,
                          int global_index);
rd_file_kw_type *
rd_file_view_iget_named_file_kw(const rd_file_view_type *rd_file_view,
                                const char *kw, int ith);
rd_kw_type *rd_file_view_iget_kw(const rd_file_view_type *rd_file_view,
                                 int index);
void rd_file_view_index_fload_kw(const rd_file_view_type *rd_file_view,
                                 const char *kw, int index,
                                 const int_vector_type *index_map,
                                 char *buffer);
int rd_file_view_find_kw_value(const rd_file_view_type *rd_file_view,
                               const char *kw, const void *value);
const char *rd_file_view_iget_distinct_kw(const rd_file_view_type *rd_file_view,
                                          int index);
int rd_file_view_get_num_distinct_kw(const rd_file_view_type *rd_file_view);
int rd_file_view_get_size(const rd_file_view_type *rd_file_view);
rd_data_type rd_file_view_iget_data_type(const rd_file_view_type *rd_file_view,
                                         int index);
int rd_file_view_iget_size(const rd_file_view_type *rd_file_view, int index);
const char *rd_file_view_iget_header(const rd_file_view_type *rd_file_view,
                                     int index);
rd_kw_type *rd_file_view_iget_named_kw(const rd_file_view_type *rd_file_view,
                                       const char *kw, int ith);
rd_data_type
rd_file_view_iget_named_data_type(const rd_file_view_type *rd_file_view,
                                  const char *kw, int ith);
int rd_file_view_iget_named_size(const rd_file_view_type *rd_file_view,
                                 const char *kw, int ith);
void rd_file_view_replace_kw(rd_file_view_type *rd_file_view,
                             rd_kw_type *old_kw, rd_kw_type *new_kw,
                             bool insert_copy);
bool rd_file_view_load_all(rd_file_view_type *rd_file_view);
void rd_file_view_add_kw(rd_file_view_type *rd_file_view,
                         rd_file_kw_type *file_kw);
void rd_file_view_free(rd_file_view_type *rd_file_view);
void rd_file_view_free__(void *arg);
int rd_file_view_get_num_named_kw(const rd_file_view_type *rd_file_view,
                                  const char *kw);
void rd_file_view_fwrite(const rd_file_view_type *rd_file_view,
                         fortio_type *target, int offset);
int rd_file_view_iget_occurence(const rd_file_view_type *rd_file_view,
                                int global_index);
void rd_file_view_fprintf_kw_list(const rd_file_view_type *rd_file_view,
                                  FILE *stream);
rd_file_view_type *rd_file_view_add_blockview(rd_file_view_type *rd_file_view,
                                              const char *header,
                                              int occurence);
rd_file_view_type *rd_file_view_add_blockview2(rd_file_view_type *rd_file_view,
                                               const char *start_kw,
                                               const char *end_kw,
                                               int occurence);
rd_file_view_type *rd_file_view_add_restart_view(rd_file_view_type *file_view,
                                                 int seqnum_index,
                                                 int report_step,
                                                 time_t sim_time,
                                                 double sim_days);
rd_file_view_type *
rd_file_view_alloc_blockview(const rd_file_view_type *rd_file_view,
                             const char *header, int occurence);
rd_file_view_type *
rd_file_view_alloc_blockview2(const rd_file_view_type *rd_file_view,
                              const char *start_kw, const char *end_kw,
                              int occurence);

void rd_file_view_add_child(rd_file_view_type *parent,
                            rd_file_view_type *child);
bool rd_file_view_drop_flag(rd_file_view_type *file_view, int flag);
void rd_file_view_add_flag(rd_file_view_type *file_view, int flag);

int rd_file_view_seqnum_index_from_sim_time(rd_file_view_type *parent_map,
                                            time_t sim_time);
bool rd_file_view_has_sim_time(const rd_file_view_type *rd_file_view,
                               time_t sim_time);
int rd_file_view_find_sim_time(const rd_file_view_type *rd_file_view,
                               time_t sim_time);
double rd_file_view_iget_restart_sim_days(const rd_file_view_type *rd_file_view,
                                          int seqnum_index);
time_t rd_file_view_iget_restart_sim_date(const rd_file_view_type *rd_file_view,
                                          int seqnum_index);
bool rd_file_view_has_report_step(const rd_file_view_type *rd_file_view,
                                  int report_step);

rd_file_view_type *rd_file_view_add_summary_view(rd_file_view_type *file_view,
                                                 int report_step);
const char *rd_file_view_get_src_file(const rd_file_view_type *file_view);
void rd_file_view_fclose_stream(rd_file_view_type *file_view);

void rd_file_view_write_index(const rd_file_view_type *file_view,
                              FILE *ostream);
rd_file_view_type *rd_file_view_fread_alloc(fortio_type *fortio, int *flags,
                                            inv_map_type *inv_map,
                                            FILE *istream);

rd_file_transaction_type *
rd_file_view_start_transaction(rd_file_view_type *file_view);
void rd_file_view_end_transaction(rd_file_view_type *file_view,
                                  rd_file_transaction_type *transaction);

#ifdef __cplusplus
}
#endif

#endif
