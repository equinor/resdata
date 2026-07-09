#pragma once
#include <cstdio>
#include <ctime>
#include <unordered_map>
#include <memory>
#include <string>

#include <ert/util/int_vector.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_flag.hpp>

using inv_map_type = std::unordered_map<const rd_kw_type *, FileKW *>;

typedef struct rd_file_view_struct rd_file_view_type;

bool rd_file_view_flags_set(const rd_file_view_type *file_view,
                            FileMode query_flags);
bool rd_file_view_check_flags(FileMode state_flags, FileMode query_flags);

rd_file_view_type *rd_file_view_alloc(ERT::FortIO *fortio, FileMode *flags,
                                      inv_map_type *inv_map);
void rd_file_view_make_index(rd_file_view_type *rd_file_view);
bool rd_file_view_has_kw(const rd_file_view_type *rd_file_view, const char *kw);
rd_kw_type *rd_file_view_iget_kw(const rd_file_view_type *rd_file_view,
                                 int index);
void rd_file_view_index_fload_kw(const rd_file_view_type *rd_file_view,
                                 const char *kw, int index,
                                 const int_vector_type *index_map,
                                 char *buffer);
const std::string &
rd_file_view_iget_distinct_kw(const rd_file_view_type *rd_file_view, int index);
int rd_file_view_get_num_distinct_kw(const rd_file_view_type *rd_file_view);
int rd_file_view_get_size(const rd_file_view_type *rd_file_view);
rd_kw_type *rd_file_view_iget_named_kw(const rd_file_view_type *rd_file_view,
                                       const char *kw, int ith);
bool rd_file_view_load_all(rd_file_view_type *rd_file_view);
void rd_file_view_free(rd_file_view_type *rd_file_view);
int rd_file_view_get_num_named_kw(const rd_file_view_type *rd_file_view,
                                  const char *kw);
void rd_file_view_fwrite(const rd_file_view_type *rd_file_view,
                         ERT::FortIO &target, int offset);
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
rd_file_view_alloc_blockview2(const rd_file_view_type *rd_file_view,
                              const char *start_kw, const char *end_kw,
                              int occurence);

bool rd_file_view_drop_flag(rd_file_view_type *file_view, FileMode flag);
void rd_file_view_add_flag(rd_file_view_type *file_view, FileMode flag);

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
rd_file_view_type *rd_file_view_fread_alloc(ERT::FortIO *fortio,
                                            FileMode *flags,
                                            inv_map_type *inv_map,
                                            FILE *istream);

void rd_file_view_clear(rd_file_view_type *file_view);

using rd_file_view_ptr =
    std::unique_ptr<rd_file_view_type, decltype(&rd_file_view_free)>;

std::shared_ptr<FileKW>
rd_file_view_iget_file_kw(const rd_file_view_type *rd_file_view,
                          int global_index);
void rd_file_view_add_kw(rd_file_view_type *rd_file_view,
                         std::shared_ptr<FileKW> file_kw);
