#pragma once
#include <ctime>

#include <memory>
#include <string>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file_flag.hpp>
#include <utility>

namespace rd {
struct File {
    std::shared_ptr<rd::FileContext> context;
    std::shared_ptr<rd::FileView>
        global_view; /* The index of all the rd_kw instances in the file. */
    File(std::shared_ptr<rd::FileContext> context,
         std::shared_ptr<rd::FileView> global_view)
        : context(std::move(context)), global_view(std::move(global_view)) {};
    static std::unique_ptr<File> open(const std::string &filename,
                                      FileMode flags = FileMode::DEFAULT);
    static std::unique_ptr<File> fast_open(const std::string &file_name,
                                           const std::string &index_file_name,
                                           FileMode flags = FileMode::DEFAULT);
    void write(ERT::FortIO &target, size_t offset) {
        global_view->write(target, offset);
    };
    bool write_index(const std::string &index_filename);
    /** The rd_file_close() function will close the fortio instance */
    void close() {
        if (context)
            context->fortio.fclose_stream();
    };
    /** true if the File has at-least one occurence of @kw. */
    [[nodiscard]] bool has_kw(const std::string &kw) const {
        return global_view->has_kw(kw);
    }
    /** The number of times @kw occurs in the File */
    [[nodiscard]] size_t num_named_kw(const std::string &kw) const {
        return global_view->num_named_kw(kw);
    }
    /** Will return the ith occurence of @kw the File. */
    [[nodiscard]] rd_kw_type *get_kw(const std::string &kw, int ith) const {
        return global_view->get_kw(kw, ith);
    }
    /** The total number of rd_kws in the File. */
    [[nodiscard]] size_t size() const { return global_view->size(); }

    [[nodiscard]] std::string filename() const {
        return context->fortio.filename();
    }
};
} // namespace rd
using rd_file_ptr = std::unique_ptr<rd::File>;
using rd_file_type = rd::File;
rd_version_enum rd_file_get_simulator_version(const rd_file_type *file);

bool rd_file_writable(const rd_file_type *rd_file);

rd_kw_type *rd_file_iget_kw(const rd_file_type *file, int global_index);
std::shared_ptr<rd::FileView>
rd_file_get_global_blockview(rd_file_type *rd_file, const char *kw,
                             int occurence);
std::shared_ptr<rd::FileView> rd_file_get_global_view(rd_file_type *rd_file);
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

bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence);
