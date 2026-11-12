#pragma once
#include <cstdio>
#include <ctime>

#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <utility>

#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/smspec_node.hpp>
#include <resdata/rd_sum_vector.hpp>
#include <resdata/rd_file_flag.hpp>

namespace rd {
class rd_sum_file_data;

/** Manages the simulated values from a summary file, including all time
    related information. In the case of restarted simulations the different
    summary cases are internalized as separate rd_sum_file_data instances. */
class SumData {
public:
    // The class CaseIndex and the struct IndexNode are used to maintain a list
    // of the rd_sum_file_data instances, and lookup the correct one based one
    // various time related arguments.
    struct IndexNode {
        size_t end() const { return this->offset + this->length; }

        size_t data_index;
        size_t offset;
        size_t length;
        size_t report1;
        size_t report2;
        time_t time1;
        time_t time2;
        double days1;
        double days2;
        std::vector<std::optional<size_t>> params_map;
    };

    class CaseIndex {
    public:
        void add(size_t data_index, const rd_sum_file_data &data,
                 std::optional<time_t> next_start,
                 const rd_smspec_type *smspec);
        std::pair<const IndexNode *, const IndexNode *>
        lookup_time(time_t sim_time) const;
        std::pair<const IndexNode *, const IndexNode *>
        lookup_days(double days) const;
        const IndexNode &lookup(size_t internal_index) const;
        const IndexNode &lookup_report(size_t report) const;
        void clear() { this->index.clear(); }
        size_t length() const {
            if (this->index.empty())
                return 0;
            return this->index.back().end();
        }
        std::vector<IndexNode>::const_iterator begin() const {
            return this->index.begin();
        }
        std::vector<IndexNode>::const_iterator end() const {
            return this->index.end();
        }

    private:
        std::vector<IndexNode> index;
    };

    /** An internal index resolved to the file holding it, and the index
        within that file. */
    struct Located {
        const IndexNode &node;
        const rd_sum_file_data &file;
        size_t local_index;
    };

    explicit SumData(const rd_smspec_type *smspec) : smspec(smspec) {}

    // Copies would share the mutable file data with the original. Declaring
    // the copy operations also suppresses the implicit moves.
    SumData(const SumData &) = delete;
    SumData &operator=(const SumData &) = delete;

    /** A SumData with a single empty file_data, ready for add_new_tstep(). */
    static std::unique_ptr<SumData> make_writer(const rd_smspec_type *smspec);

    const rd_smspec_type *smspec;
    /** These data files are shared_ptr as rd_sum_data_add_case makes
        file_data ownership shared among rd_sum_data. */
    std::vector<std::shared_ptr<rd_sum_file_data>> data_files;
    CaseIndex index;

    const rd_sum_file_data &file(const IndexNode &node) const;
    Located locate(size_t internal_index) const;
    double default_value(size_t params_index) const;
    double iget(size_t time_index, size_t params_index) const;

    /** Reads the values of @params_indices at @time_index into @out, which
        must hold params_indices.size() values. Lazily loaded data is read
        from file once per call, instead of once per value as with iget(). */
    void iget_row(size_t time_index, const std::vector<size_t> &params_indices,
                  double *out) const;
    double iget_sim_seconds(size_t internal_index) const;
    void build_index();

private:
    void check_params_index(size_t params_index) const;
};
} // namespace rd

typedef rd::SumData rd_sum_data_type;

void rd_sum_data_add_case(rd_sum_data_type *self,
                          const rd_sum_data_type *other);
void rd_sum_data_fwrite(const rd_sum_data_type *data, const char *rd_case,
                        bool fmt_case, bool unified);
bool rd_sum_data_can_write(const rd_sum_data_type *data);
bool rd_sum_data_fread(rd_sum_data_type *data,
                       const std::vector<std::string> &filelist, bool lazy_load,
                       FileMode file_options);
inline rd_sum_data_type *rd_sum_data_alloc_writer(rd_smspec_type *smspec) {
    return rd::SumData::make_writer(smspec).release();
}
inline rd_sum_data_type *rd_sum_data_alloc(rd_smspec_type *smspec) {
    return new rd::SumData(smspec);
}
std::optional<size_t>
rd_sum_data_get_report_step_from_time(const rd_sum_data_type *data,
                                      time_t sim_time);
std::optional<size_t>
rd_sum_data_get_report_step_from_days(const rd_sum_data_type *data,
                                      double days);
bool rd_sum_data_check_sim_time(const rd_sum_data_type *data, time_t sim_time);
bool rd_sum_data_check_sim_days(const rd_sum_data_type *data, double sim_days);
/** One value per time step. Throws std::out_of_range for an invalid index. */
std::vector<double> rd_sum_data_alloc_data_vector(const rd_sum_data_type *data,
                                                  size_t params_index);
/** One value per report step present in the data. */
std::vector<double>
rd_sum_data_alloc_report_data_vector(const rd_sum_data_type *data,
                                     size_t params_index);
std::vector<time_t> rd_sum_data_alloc_time_vector(const rd_sum_data_type *data);
std::vector<time_t>
rd_sum_data_alloc_report_time_vector(const rd_sum_data_type *data);
time_t rd_sum_data_get_data_start(const rd_sum_data_type *data);
time_t rd_sum_data_get_report_time(const rd_sum_data_type *data,
                                   size_t report_step);
double rd_sum_data_get_first_day(const rd_sum_data_type *data);
time_t rd_sum_data_get_sim_end(const rd_sum_data_type *data);
double rd_sum_data_get_sim_length(const rd_sum_data_type *data);
double rd_sum_data_iget(const rd_sum_data_type *data, size_t internal_index,
                        size_t params_index);

double rd_sum_data_iget_sim_days(const rd_sum_data_type *, size_t);
time_t rd_sum_data_iget_sim_time(const rd_sum_data_type *, size_t);
std::vector<double>
rd_sum_data_get_interp_vector(const rd_sum_data_type *data, time_t sim_time,
                              const rd_sum_vector_type *keylist,
                              double missing_value);

inline void rd_sum_data_free(rd_sum_data_type *data) { delete data; }
size_t rd_sum_data_get_last_report_step(const rd_sum_data_type *data);
size_t rd_sum_data_get_first_report_step(const rd_sum_data_type *data);

double rd_sum_data_get_from_sim_time(const rd_sum_data_type *data,
                                     time_t sim_time,
                                     const rd::smspec_node &smspec_node);
double rd_sum_data_get_from_sim_days(const rd_sum_data_type *data,
                                     double sim_days,
                                     const rd::smspec_node &smspec_node);

size_t rd_sum_data_get_length(const rd_sum_data_type *data);
size_t rd_sum_data_iget_report_step(const rd_sum_data_type *data,
                                    size_t internal_index);
size_t rd_sum_data_iget_report_end(const rd_sum_data_type *data,
                                   size_t report_step);
rd_sum_tstep_type *rd_sum_data_add_new_tstep(rd_sum_data_type *data,
                                             size_t report_step,
                                             double sim_seconds);
double rd_sum_data_iget_last_value(const rd_sum_data_type *data,
                                   size_t param_index);
double rd_sum_data_iget_first_value(const rd_sum_data_type *data,
                                    size_t param_index);
void rd_sum_data_init_double_vector(const rd_sum_data_type *data,
                                    size_t params_index, double *output_data);
std::vector<double>
rd_sum_data_alloc_seconds_solution(const rd_sum_data_type *data,
                                   const rd::smspec_node &node, double value,
                                   bool rates_clamp_lower);
void rd_sum_data_init_double_frame_interp(
    const rd_sum_data_type *data, const rd_sum_vector_type *keywords,
    const std::vector<time_t> &time_points, double *output_data);
void rd_sum_data_init_double_frame(const rd_sum_data_type *data,
                                   const rd_sum_vector_type *keywords,
                                   double *output_data);

void rd_sum_data_init_double_vector_interp(
    const rd_sum_data_type *data, const rd::smspec_node &smspec_node,
    const std::vector<time_t> &time_points, double *output_data);

using rd_sum_data_ptr =
    std::unique_ptr<rd_sum_data_type, decltype(&rd_sum_data_free)>;
