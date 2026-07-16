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
    [[nodiscard]] bool is_writable() const {
        return (context->flags & FileMode::WRITABLE) == FileMode::WRITABLE;
    }
    std::shared_ptr<rd::FileView> blockview(const std::string &kw,
                                            size_t occurence) {
        return global_view->blockview(kw, kw, occurence);
    }
    std::shared_ptr<rd::FileView> summary_view(int report_step) {
        return global_view->summary_view(report_step);
    }

    [[nodiscard]] std::shared_ptr<rd::FileView> get_global_view() const {
        return global_view;
    }

    /** Will save the content of @rd_kw to the File.

        This function is quite strict:
        1. The actual keyword which should be updated is identified using
           pointer comparison; i.e. the rd_kw argument must be the actual
           return value from an earlier get_kw() operation.

        2. The header data of the rd_kw must be unmodified, and will throw
           std::runtime_error if there is a mismatch.

        3. The File must have been opened with FileMode::WRITABLE. */
    bool save_kw(const rd_kw_type *rd_kw);
    double restart_sim_days(int index) {
        return global_view->restart_sim_days(index);
    }
};
} // namespace rd
using rd_file_ptr = std::unique_ptr<rd::File>;
using rd_file_type = rd::File;

time_t rd_file_iget_restart_sim_date(const rd_file_type *restart_file,
                                     int occurence);
int rd_file_get_restart_index(const rd_file_type *restart_file,
                              time_t sim_time);
bool rd_file_has_report_step(const rd_file_type *rd_file, int report_step);
bool rd_file_has_sim_time(const rd_file_type *rd_file, time_t sim_time);

bool rd_file_subselect_block(rd_file_type *rd_file, const char *kw,
                             int occurence);
