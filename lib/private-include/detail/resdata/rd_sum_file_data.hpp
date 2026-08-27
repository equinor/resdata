#include <algorithm>
#include <ctime>
#include <utility>
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <stdexcept>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_file_view.hpp>

namespace rd {

struct IndexNode {

    IndexNode(time_t sim_time, double sim_seconds, size_t report_step)
        : sim_time(sim_time), sim_seconds(sim_seconds),
          report_step(report_step) {}

    time_t sim_time;
    double sim_seconds;
    size_t report_step;
};

struct ReportRange {
    size_t first;
    size_t last;
};

class TimeIndex {
public:
    void add(time_t sim_time, double sim_seconds, size_t report_step) {
        size_t internal_index = this->nodes.size();
        this->nodes.emplace_back(sim_time, sim_seconds, report_step);

        /* Indexing internal_index - report_step */
        if (this->report_map.size() <= report_step)
            this->report_map.resize(report_step + 1, std::nullopt);

        auto &range = this->report_map[report_step];
        if (range) {
            range->first = std::min(range->first, internal_index);
            range->last = std::max(range->last, internal_index);
        } else
            range = ReportRange{internal_index, internal_index};
    }

    bool has_report(size_t report_step) const {
        if (report_step >= this->report_map.size())
            return false;

        return this->report_map[report_step].has_value();
    }

    void clear() {
        this->nodes.clear();
        this->report_map.clear();
    }

    const IndexNode &operator[](size_t index) const {
        return this->nodes.at(index);
    }

    const IndexNode &back() const { return this->nodes.back(); }

    bool empty() const { return this->nodes.empty(); }

    size_t size() const { return this->nodes.size(); }

    /** Returns std::nullopt when @report_step is not present in this file. */
    std::optional<ReportRange> report_range(size_t report_step) const {
        if (report_step >= this->report_map.size())
            return std::nullopt;

        return this->report_map[report_step];
    }

private:
    std::vector<IndexNode> nodes;
    std::vector<std::optional<ReportRange>> report_map;
};

class unsmry_loader;

class rd_sum_file_data {

public:
    rd_sum_file_data(const rd_smspec_type *smspec);
    ~rd_sum_file_data();
    const rd_smspec_type *smspec() const;

    size_t length_before(time_t end_time) const;
    void get_time(size_t length, time_t *data);
    void get_data(size_t params_index, size_t length, double *data);
    size_t length() const;
    time_t get_data_start() const;
    time_t get_sim_end() const;
    double iget(size_t time_index, size_t params_index) const;
    time_t iget_sim_time(size_t time_index) const;
    double iget_sim_days(size_t time_index) const;
    double iget_sim_seconds(size_t time_index) const;
    rd_sum_tstep_type *iget_ministep(size_t internal_index) const;
    double get_days_start() const;
    double get_sim_length() const;

    /** Returns std::nullopt when @report_step is not present in this file. */
    std::optional<ReportRange> report_range(size_t report_step) const;

    /** Returns the last report step strictly before @end_time */
    size_t report_before(time_t end_time) const;
    size_t get_time_report(size_t max_internal_index, time_t *data);
    size_t get_data_report(std::optional<size_t> params_index,
                           size_t max_internal_index, double *data,
                           double default_value);
    size_t first_report() const;
    size_t last_report() const;
    size_t iget_report(size_t time_index) const;
    bool has_report(size_t report_step) const;
    std::optional<size_t> report_step_from_days(double sim_days) const;
    std::optional<size_t> report_step_from_time(time_t sim_time) const;

    rd_sum_tstep_type *add_new_tstep(size_t report_step, double sim_seconds);
    bool can_write() const;
    void fwrite_unified(ERT::FortIO &fortio) const;
    void fwrite_multiple(const std::string &rd_case, bool fmt_case) const;
    bool fread(const std::vector<std::string> &filelist, bool lazy_load,
               FileMode file_options = FileMode::DEFAULT);

private:
    const rd_smspec_type *rd_smspec;

    TimeIndex index;
    std::vector<rd_sum_tstep_ptr> data;

    std::unique_ptr<rd::unsmry_loader> loader;

    void append_tstep(rd_sum_tstep_ptr tstep);
    void build_index();
    void fwrite_report(size_t report_step, ERT::FortIO &fortio) const;
    bool check_file(rd::File *rd_file);
    void add_rd_file(size_t report_step, rd::FileView &summary_view);
};

} // namespace rd
