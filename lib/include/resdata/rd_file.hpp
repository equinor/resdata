#pragma once
#include <ctime>

#include <filesystem>
#include <memory>
#include <string>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file_flag.hpp>

#include "ert/util/type_macros.hpp"

typedef struct rd_file_struct rd_file_type;
struct rd_file_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::shared_ptr<rd::FileContext> context;
    std::shared_ptr<rd::FileView>
        global_view; /* The index of all the rd_kw instances in the file. */
    std::shared_ptr<rd::FileView> active_view; /* The currently active index. */
};
bool rd_file_load_all(rd_file_type *rd_file);
rd_file_type *rd_file_open(const char *filename,
                           FileMode flags = FileMode::DEFAULT);
rd_file_type *rd_file_fast_open(const char *filename,
                                const char *index_filename,
                                FileMode flags = FileMode::DEFAULT);
bool rd_file_write_index(const rd_file_type *rd_file,
                         const char *index_filename);
bool rd_file_index_valid(const char *file_name, const char *index_file_name);
void rd_file_close(rd_file_type *rd_file);
void rd_file_free(rd_file_type *rd_file);
rd_kw_type *rd_file_icopy_kw(const rd_file_type *rd_file, int index);
bool rd_file_has_kw(const rd_file_type *rd_file, const char *kw);
int rd_file_get_num_named_kw(const rd_file_type *rd_file, const char *kw);
int rd_file_get_size(const rd_file_type *rd_file);
const char *rd_file_get_src_file(const rd_file_type *rd_file);
rd_version_enum rd_file_get_rd_version(const rd_file_type *file);
void rd_file_fwrite_fortio(const rd_file_type *ec_file, ERT::FortIO &fortio,
                           size_t offset);

int rd_file_get_phases(const rd_file_type *init_file);

bool rd_file_writable(const rd_file_type *rd_file);

rd_kw_type *rd_file_iget_kw(const rd_file_type *file, int global_index);
rd_kw_type *rd_file_iget_named_kw(const rd_file_type *file, const char *kw,
                                  int ith);
std::shared_ptr<rd::FileView>
rd_file_get_global_blockview(rd_file_type *rd_file, const char *kw,
                             int occurence);
std::shared_ptr<rd::FileView> rd_file_get_global_view(rd_file_type *rd_file);
std::shared_ptr<rd::FileView> rd_file_get_active_view(rd_file_type *rd_file);
bool rd_file_save_kw(const rd_file_type *rd_file, const rd_kw_type *rd_kw);

double rd_file_iget_restart_sim_days(const rd_file_type *restart_file,
                                     int index);
time_t rd_file_iget_restart_sim_date(const rd_file_type *restart_file,
                                     int occurence);
int rd_file_get_restart_index(const rd_file_type *restart_file,
                              time_t sim_time);
bool rd_file_has_report_step(const rd_file_type *rd_file, int report_step);
bool rd_file_has_sim_time(const rd_file_type *rd_file, time_t sim_time);

std::shared_ptr<rd::FileView> rd_file_get_summary_view(rd_file_type *rd_file,
                                                       int report_step);

UTIL_IS_INSTANCE_HEADER(rd_file);

bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence);

using rd_file_ptr = std::unique_ptr<rd_file_type, decltype(&rd_file_free)>;

inline rd_file_ptr open_rd_file(const std::string &path,
                                FileMode flags = FileMode::DEFAULT) {
    return {rd_file_open(path.c_str(), flags), &rd_file_free};
}
inline rd_file_ptr open_rd_file(const std::filesystem::path &path,
                                FileMode flags = FileMode::DEFAULT) {
    return {rd_file_open(path.string().c_str(), flags), &rd_file_free};
}
