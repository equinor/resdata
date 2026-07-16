#pragma once
#include <ctime>

#include <memory>
#include <string>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/FortIO.hpp>
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
    [[nodiscard]] rd_kw_type *get_kw(const std::string &kw, size_t ith) const {
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

    /// Functions specialized to work with restart files.

    double restart_sim_days(int index) {
        return global_view->restart_sim_days(index);
    }
    time_t restart_sim_date(int index) {
        return global_view->restart_sim_date(index);
    }
    /** Will determine the restart block corresponding to @sim_time;
       if @sim_time can not be found the function -1 is returned.

        The returned index is the 'occurence number' in the restart file,
        i.e. in the (quite typical case) that not all report steps are
        present the return value will not agree with report step.

        The return value from this function can then be used to get a
        corresponding solution field directly, or the file map can
        restricted to this block.

        Direct access:

           int index = rd_file->find_sim_time(sim_time);
           if (index >= 0) {
              rd_kw_type * pressure_kw = rd_file->get_kw("PRESSURE", index );
              ....
           }

        In the case of LGRs the block restriction should be used. */
    int find_sim_time(time_t sim_time) {
        return global_view->find_sim_time(sim_time);
    }

    /** Will look through all the SEQNUM kw instances of the current
        rd_file and look for @report_step. If the value is found true is
        returned, otherwise false. */
    bool has_report_step(int report_step) {
        return global_view->has_report_step(report_step);
    }
    /** Will look through all the INTEHEAD kw instances of the current
        rd_file and look for @sim_time. If the value is found true is
        returned, otherwise false. */
    bool has_sim_time(time_t sim_time) {
        return global_view->has_sim_time(sim_time);
    }
};
} // namespace rd
