#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <fmt/core.h>
#include <ios>
#include <optional>
#include <utility>
#include <vector>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <memory>

#include <ert/util/util.hpp>

#include <fmt/format.h>

#include <resdata/rd_util.hpp>
#include <resdata/rd_smspec.hpp>
#include "detail/resdata/rd_sum_data.hpp"
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/smspec_node.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_sum_vector.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/FortIO.hpp>

#include "detail/resdata/rd_sum_file_data.hpp"

namespace fs = std::filesystem;

// This file implements the class rd::SumData which manages the actual simulated
// values from a summary file, including all time-related information. In the
// case of restarted simulations the different summary cases will be internalized
// as separate rd_sum_file_data instances. The rd_sum_file_data class is an
// internal implementation detail which is not exported. More details about the
// internal storage of summary data can be found in that file.
//
// For this file the implementation mainly consists of maintaining an ordered
// list of rd_sum_file_data instances and dispatching method calls to the right
// rd_sum_file_data instance.

namespace rd {

/** Adds the data file with index @data_index in the list of data files. If
    @next_start is given, the data file is truncated to the timesteps before
    @next_start. Data files which contribute no timesteps are not added, so
    every node in the index has length > 0. */
void SumData::CaseIndex::add(size_t data_index, const rd_sum_file_data &data,
                             std::optional<time_t> next_start,
                             const rd_smspec_type *smspec) {
    size_t length =
        next_start ? data.length_before(*next_start) : data.length();
    if (length == 0)
        return;

    size_t offset = this->index.empty() ? 0 : this->index.back().end();
    this->index.push_back(IndexNode{
        data_index,
        offset,
        length,
        data.first_report(),
        next_start ? data.report_before(*next_start) : data.last_report(),
        data.get_data_start(),
        data.get_sim_end(),
        data.get_days_start(),
        data.get_sim_length(),
        rd_smspec_alloc_mapping(smspec, data.smspec()),
    });
}

/** The lookup_time() methods will lookup which file_data
    instance corresponds to the time argument. The methods will return two
    pointers to file_data instances, if the argument is inside one file_data
    instance the pointers will be equal - otherwise they will point to the
    file_data instance before and after the argument:

    File 1                     File 2
    |------|-----|------|      |----|----------|---|
         ^                  ^
         |                  |
         |                  |
         A                  B

    For time A the lookup_time function will return <file1,file1> whereas for time
    B the function will return <file1,file2>. */
std::pair<const SumData::IndexNode *, const SumData::IndexNode *>
SumData::CaseIndex::lookup_time(time_t sim_time) const {
    if (this->index.empty())
        throw std::invalid_argument("Simulation time out of range");

    auto iter = this->index.begin();
    auto next = this->index.begin();
    if (sim_time < iter->time1)
        throw std::invalid_argument("Simulation time out of range");

    ++next;
    while (true) {
        time_t t1 = iter->time1;
        time_t t2 = iter->time2;

        if (sim_time >= t1) {
            if (sim_time <= t2)
                return std::make_pair<const IndexNode *, const IndexNode *>(
                    &(*iter), &(*iter));

            if (next == this->index.end())
                throw std::invalid_argument("Simulation days out of range");

            if (sim_time < next->time1)
                return std::make_pair<const IndexNode *, const IndexNode *>(
                    &(*iter), &(*next));
        }
        ++next;
        ++iter;
    }
}

/** Same as lookup_time, but for number of days */
std::pair<const SumData::IndexNode *, const SumData::IndexNode *>
SumData::CaseIndex::lookup_days(double days) const {
    if (this->index.empty())
        throw std::invalid_argument("Simulation days out of range");

    auto iter = this->index.begin();
    auto next = this->index.begin();
    if (days < iter->days1)
        throw std::invalid_argument("Simulation days out of range");

    ++next;
    while (true) {
        double d1 = iter->days1;
        double d2 = iter->days2;

        if (days >= d1) {
            if (days <= d2)
                return std::make_pair<const IndexNode *, const IndexNode *>(
                    &(*iter), &(*iter));

            if (next == this->index.end())
                throw std::invalid_argument("Simulation days out of range");

            if (days < next->days1)
                return std::make_pair<const IndexNode *, const IndexNode *>(
                    &(*iter), &(*next));
        }
        ++next;
        ++iter;
    }
}

const SumData::IndexNode &
SumData::CaseIndex::lookup(size_t internal_index) const {
    for (const auto &node : this->index)
        if (internal_index >= node.offset && internal_index < node.end())
            return node;

    throw std::invalid_argument("Internal error when looking up index: " +
                                std::to_string(internal_index));
}

const SumData::IndexNode &
SumData::CaseIndex::lookup_report(size_t report) const {
    for (const auto &node : this->index)
        if (node.report1 <= report && node.report2 >= report)
            return node;

    throw std::invalid_argument("Internal error when looking up report: " +
                                std::to_string(report));
}

std::unique_ptr<SumData> SumData::make_writer(const rd_smspec_type *smspec) {
    auto data = std::make_unique<SumData>(smspec);
    data->data_files.push_back(std::make_shared<rd_sum_file_data>(smspec));
    data->build_index();
    return data;
}

const rd_sum_file_data &SumData::file(const IndexNode &node) const {
    return *this->data_files[node.data_index];
}

SumData::Located SumData::locate(size_t internal_index) const {
    const auto &node = this->index.lookup(internal_index);
    return {node, this->file(node), internal_index - node.offset};
}

double SumData::default_value(size_t params_index) const {
    return rd_smspec_iget_node_w_params_index(this->smspec, params_index)
        .get_default();
}

void SumData::iget_row(size_t time_index,
                       const std::vector<size_t> &params_indices,
                       double *out) const {
    const auto loc = this->locate(time_index);

    std::vector<size_t> file_indices;
    std::vector<size_t> positions;
    file_indices.reserve(params_indices.size());
    positions.reserve(params_indices.size());
    if (!params_indices.empty())
        this->check_params_index(
            *std::max_element(params_indices.begin(), params_indices.end()));
    for (size_t i = 0; i < params_indices.size(); i++) {
        if (const auto file_index = loc.node.params_map[params_indices[i]]) {
            file_indices.push_back(*file_index);
            positions.push_back(i);
        } else
            out[i] = this->default_value(params_indices[i]);
    }

    std::vector<double> values(file_indices.size());
    loc.file.get_row(loc.local_index, file_indices, values.data());
    for (size_t j = 0; j < positions.size(); j++)
        out[positions[j]] = values[j];
}

double SumData::iget_sim_seconds(size_t internal_index) const {
    const auto loc = this->locate(internal_index);
    return loc.file.iget_sim_seconds(loc.local_index);
}

void SumData::build_index() {
    std::sort(this->data_files.begin(), this->data_files.end(),
              [](const std::shared_ptr<rd_sum_file_data> &case1,
                 const std::shared_ptr<rd_sum_file_data> &case2) {
                  return case1->get_data_start() < case2->get_data_start();
              });

    this->index.clear();
    for (size_t i = 0; i < this->data_files.size(); i++) {
        std::optional<time_t> next_start = std::nullopt;
        if (i + 1 < this->data_files.size())
            next_start = this->data_files[i + 1]->get_data_start();

        this->index.add(i, *this->data_files[i], next_start, this->smspec);
    }
}

void SumData::check_params_index(size_t params_index) const {
    const size_t params_size = rd_smspec_get_params_size(this->smspec);
    if (params_index >= params_size)
        throw std::out_of_range(
            fmt::format("params_index:{} is out of range - valid range: "
                        "[0, {})",
                        params_index, params_size));
}
} // namespace rd

using rd::SumData;

void SumData::fwrite_unified(const fs::path &rd_case, bool fmt_case) const {
    std::string filename =
        rd::filename(rd_case, FileType::UNIFIED_SUMMARY, fmt_case, 0).string();
    ERT::FortIO fortio(filename, std::ios_base::out, fmt_case);

    for (auto &data_file : this->data_files)
        data_file->fwrite_unified(fortio);
}

void SumData::fwrite_multiple(const fs::path &rd_case, bool fmt_case) const {
    for (auto &data_file : this->data_files)
        data_file->fwrite_multiple(rd_case.string(), fmt_case);
}

/** The internal index of the ministep corresponding to @sim_time. Throws
    std::out_of_range if @sim_time is before the start or after the end of the
    data; use check_sim_time() to test for this beforehand.

    Observe that if there are holes in the time-domain, i.e. if RPTONLY has
    been used, the returned ministep need not cover @sim_time:

      The 'X' should represent report times - the dashed lines
      represent the temporal extent of two ministeps. Outside the '--'
      area we do not have any results. The two ministeps we actually
      have are M15 and M25, i.e. there is a hole.


       X      .      +-----X            +----X
             /|\        M15               M25
              |
              |

      When asking for the ministep at the location of the arrow, the function
      returns the index of M15, i.e. the valid ministep following @sim_time.
      Of course - the ideal situation is if the time sequence has no holes. */
size_t SumData::get_index_from_sim_time(time_t sim_time) const {
    if (!this->check_sim_time(sim_time)) {
        time_t start_time = this->get_data_start();
        time_t end_time = this->get_sim_end();
        throw std::out_of_range(fmt::format(
            "Invalid time_t instance:{} interval:[{},{}] (simulation start:{})",
            sim_time, start_time, end_time,
            rd_smspec_get_start_time(this->smspec)));
    }

    // The moment we have passed the intial test we MUST find a valid
    // ministep index, however care should be taken that there can
    // perfectly well be 'holes' in the time domain, because of e.g. the
    // RPTONLY keyword.
    const size_t length = this->get_length();
    if (length == 0)
        throw std::logic_error("reached found time_t in empty data");

    size_t low_index = 0;
    size_t high_index = length - 1;

    // perform binary search
    while (low_index + 1 < high_index) {
        size_t center_index = (low_index + high_index) / 2;
        const time_t center_time = this->iget_sim_time(center_index);

        if (sim_time > center_time)
            low_index = center_index;
        else
            high_index = center_index;
    }

    return sim_time <= this->iget_sim_time(low_index) ? low_index : high_index;
}

/** The two ministeps bracketing a time, with the weights to interpolate
    between them. Rates are not interpolated; see interp_get(). */
struct SumData::Interp {
    size_t index1;
    size_t index2;
    double weight1;
    double weight2;

    /** Weighted average of the values at index1 and index2; rates use a step
        function, i.e. the value at index2, the ministep at or after the time. */
    double interpolate(double value1, double value2, bool is_rate) const {
        return is_rate ? value2 : value1 * weight1 + value2 * weight2;
    }
};

SumData::Interp SumData::interp_from_sim_time(time_t sim_time) const {
    const size_t idx = this->get_index_from_sim_time(sim_time);

    // if sim_time is first date, idx=0 and then we cannot interpolate, so we give
    // weight 1 to index1=index2=0.
    if (idx == 0)
        return {0, 0, 1, 0};

    const time_t sim_time1 = this->iget_sim_time(idx - 1);
    const time_t sim_time2 = this->iget_sim_time(idx);

    // weights the interpolation each of the ministeps according to distance from sim_time
    const double time_diff = static_cast<double>(sim_time2 - sim_time1);
    const double time_dist1 = static_cast<double>(sim_time - sim_time1);
    const double time_dist2 = -static_cast<double>(sim_time - sim_time2);

    return {idx - 1, idx, time_dist2 / time_diff, time_dist1 / time_diff};
}

/** The value of @params_index at @sim_time given by @interp; only the ministeps
    needed (index2 alone for rates) are read. */
double SumData::interp_get(const Interp &interp, size_t params_index,
                           bool is_rate) const {
    if (is_rate)
        return this->iget(interp.index2, params_index);
    return interp.interpolate(this->iget(interp.index1, params_index),
                              this->iget(interp.index2, params_index), false);
}

namespace {
/** The params indices of the keys of @keywords present in the summary, and
    the indices of those keys in @keywords. */
std::pair<std::vector<size_t>, std::vector<size_t>>
present_keywords(const rd_sum_vector_type *keywords) {
    std::vector<size_t> params_indices;
    std::vector<size_t> key_indices;
    const size_t num_keywords = rd_sum_vector_get_size(keywords);
    for (size_t i = 0; i < num_keywords; i++)
        if (auto params_index = rd_sum_vector_iget_param_index(keywords, i)) {
            params_indices.push_back(*params_index);
            key_indices.push_back(i);
        }
    return {std::move(params_indices), std::move(key_indices)};
}
} // namespace

/** The values of the keywords present in the summary at one ministep, read
    with a single iget_row() call. */
struct SumData::RowCache {
    std::optional<size_t> time_index = std::nullopt;
    std::vector<double> values;

    void load(const SumData &data, size_t new_time_index,
              const std::vector<size_t> &params_indices) {
        if (time_index == new_time_index)
            return;
        values.resize(params_indices.size());
        data.iget_row(new_time_index, params_indices, values.data());
        time_index = new_time_index;
    }

    double operator[](size_t i) const { return values[i]; }
};

double SumData::iget_sim_days(size_t internal_index) const {
    const auto loc = this->locate(internal_index);
    return loc.file.iget_sim_days(loc.local_index);
}

void SumData::fwrite(const char *rd_case, bool fmt_case, bool unified) const {
    fs::path case_path(rd_case);
    if (unified)
        this->fwrite_unified(case_path, fmt_case);
    else
        this->fwrite_multiple(case_path, fmt_case);
}

bool SumData::can_write() const {
    bool can_write = true;
    for (const auto &file_ptr : this->data_files)
        can_write &= file_ptr->can_write();

    return can_write;
}

time_t SumData::get_sim_end() const {
    if (this->data_files.empty())
        throw std::out_of_range("SumData::get_sim_end: data_files empty");

    const auto &file_data = this->data_files.back();
    return file_data->get_sim_end();
}

time_t SumData::get_data_start() const {
    if (this->data_files.empty())
        throw std::out_of_range("SumData::get_data_start: data_files empty");
    const auto &file_data = this->data_files[0];
    return file_data->get_data_start();
}

double SumData::get_first_day() const {
    const auto &file_data = this->data_files[0];
    return file_data->get_days_start();
}

/** Returns the number of simulations days from the start of the
    simulation (irrespective of whether the that summary data has
    actually been loaded) to the last loaded simulation step. */
double SumData::get_sim_length() const {
    const auto &file_data = this->data_files.back();
    return file_data->get_sim_length();
}

/** The check_sim_time() and check_sim_days() functions check if you
    have summary data for the requested date/days value. In the case of
    a restarted case, where the original case is missing - this will
    return false if the input values are in the region after simulation
    start with no data. */
bool SumData::check_sim_time(time_t sim_time) const {
    if (sim_time < this->get_data_start())
        return false;

    if (sim_time > this->get_sim_end())
        return false;

    return true;
}

bool SumData::check_sim_days(double sim_days) const {
    return sim_days >= this->get_first_day() &&
           sim_days <= this->get_sim_length();
}

std::vector<double>
SumData::alloc_seconds_solution(const rd::smspec_node &node, double cmp_value,
                                bool rates_clamp_lower) const {
    std::vector<double> solution;
    const size_t param_index = node.get_params_index();
    const size_t size = this->get_length();

    if (size <= 1)
        return solution;

    for (size_t index = 0; index < size; ++index) {
        size_t prev_index = (index == 0) ? 0 : index - 1;
        double value = this->iget(index, param_index);
        double prev_value = this->iget(prev_index, param_index);

        // cmp_value in interval value (closed) and prev_value (open)
        bool contained = (value == cmp_value);
        contained |= (std::min(prev_value, value) < cmp_value) &&
                     (cmp_value < std::max(prev_value, value));

        if (!contained)
            continue;

        double prev_time = this->iget_sim_seconds(prev_index);
        double time = this->iget_sim_seconds(index);

        if (node.is_rate()) {
            solution.push_back(rates_clamp_lower ? prev_time + 1 : time);
        } else {
            double slope = (value - prev_value) / (time - prev_time);
            double seconds = (cmp_value - prev_value) / slope + prev_time;
            solution.push_back(seconds);
        }
    }
    return solution;
}

/** This function is meant to be called in write mode; and will create a
    new and empty tstep which is appended to the current data. The tstep
    will also be returned, so the calling scope can call
    rd_sum_tstep_iset() to set elements in the tstep. */
rd_sum_tstep_type *SumData::add_new_tstep(size_t report_step,
                                          double sim_seconds) {
    const auto &file_data = this->data_files.back();
    rd_sum_tstep_type *tstep =
        file_data->add_new_tstep(report_step, sim_seconds);
    this->build_index();
    return tstep;
}

void SumData::add_case(const SumData &other) {
    for (auto &other_file : other.data_files)
        this->data_files.push_back(other_file);

    this->build_index();
}

/** Loads the summary files in @filelist and appends them to the data. The
    index is rebuilt on success. Returns false, leaving the data unchanged, if
    the files could not be read. */
bool SumData::fread(const std::vector<std::string> &filelist, bool lazy_load,
                    FileMode file_options) {
    auto file_data = std::make_shared<rd_sum_file_data>(this->smspec);
    if (file_data->fread(filelist, lazy_load, file_options)) {
        this->data_files.push_back(file_data);
        this->build_index();
        return true;
    }
    return false;
}

/** The last index included in report step @report_step. If the dataset does
    not contain @report_step, the function will raise std::invalid_argument. */
size_t SumData::iget_report_end(size_t report_step) const {
    const auto &index_node = this->index.lookup_report(report_step);
    auto &file_data = this->file(index_node);
    const auto range = file_data.report_range(report_step);
    if (!range)
        throw std::invalid_argument("Could not locate report step " +
                                    std::to_string(report_step) +
                                    " within its summary file");

    size_t end = range->last;
    if (end >= index_node.length) {
        // report_range() returns the report's extent within the file, but
        // if the time of steps overlap between files, the final timesteps are
        // truncated because the next file takes over, so we use the last step
        // in the file.
        end = index_node.length - 1;
    }

    if (file_data.iget_report(end) != report_step)
        throw std::invalid_argument(
            "Could not locate report step " + std::to_string(report_step) +
            " within the retained range of its summary file");

    return end + index_node.offset;
}

size_t SumData::iget_report_step(size_t internal_index) const {
    const auto loc = this->locate(internal_index);
    return loc.file.iget_report(loc.local_index);
}

/** This will look up a value based on an internal index. The internal
    index will ALWAYS run in the interval [0,num_ministep), without
    any holes. */
double SumData::iget(size_t time_index, size_t params_index) const {
    const auto loc = this->locate(time_index);
    this->check_params_index(params_index);
    // nullopt if the file lacks the parameter, so the default value applies
    const auto file_index = loc.node.params_map[params_index];
    return file_index ? loc.file.iget(loc.local_index, *file_index)
                      : this->default_value(params_index);
}

/** If the keylist contains invalid indices the corresponding element in the
    results vector will be set to missing_value. */
std::vector<double>
SumData::get_interp_vector(time_t sim_time, const rd_sum_vector_type *keylist,
                           double missing_value) const {
    const size_t num_keywords = rd_sum_vector_get_size(keylist);
    const Interp interp = this->interp_from_sim_time(sim_time);

    const auto [params_indices, key_indices] = present_keywords(keylist);

    RowCache row1, row2;
    row1.load(*this, interp.index1, params_indices);
    row2.load(*this, interp.index2, params_indices);

    std::vector<double> results(num_keywords, missing_value);
    for (size_t j = 0; j < key_indices.size(); j++) {
        const size_t i = key_indices[j];
        results[i] = interp.interpolate(row1[j], row2[j],
                                        rd_sum_vector_iget_is_rate(keylist, i));
    }
    return results;
}

double SumData::get_from_sim_time(time_t sim_time,
                                  const rd::smspec_node &smspec_node) const {
    size_t params_index = smspec_node.get_params_index();
    if (smspec_node.is_rate()) {
        // In general the mapping from sim_time to index is based on half
        // open intervals, which are closed in the upper end:
        //
        //     []<------------]<--------------]<-----------]
        //     t0             t1             t2           t3
        //
        //  However - as indicated on the figure above there is a zero
        //  measure point right at the start which corresponds to
        //  time_index == 0; this is to ensure that there is correspondence
        //  with the simulator results if you ask for a value interpolated to
        //  the starting time.
        size_t time_index = this->get_index_from_sim_time(sim_time);
        return this->iget(time_index, params_index);
    } else {
        // Interpolated lookup based on two (hopefully) consecutive ministeps.
        return this->interp_get(this->interp_from_sim_time(sim_time),
                                params_index, false);
    }
}

std::optional<size_t>
SumData::get_report_step_from_days(double sim_days) const {
    if ((sim_days < this->get_first_day()) ||
        (sim_days > this->get_sim_length()))
        return std::nullopt;
    else {
        auto files = this->index.lookup_days(sim_days);
        if (files.first != files.second)
            return std::nullopt;

        return this->file(*files.first).report_step_from_days(sim_days);
    }
}

/** Will go through the data and find the report step which EXACTLY
    matches the input sim_time. If no report step matches exactly the
    function will return std::nullopt.

    Observe that by default the report steps consist of half-open time
    intervals like this: (t1, t2]. However the first report step
    (i.e. report step 1, is a fully inclusive interval: [t0 , t1] where
    t0 is the simulation start time. That is not implemented here;
    meaning that if you supply the start time as @sim_time argument you
    will get std::nullopt and not 0 as you might expect.

    It would certainly be possible to detect the start_time input
    argument and special case the return, but the opposite would be
    'impossible' - you would never get anything sensible out when using
    report_step == 0 as input to one of the functions expecting
    report_step input. */
std::optional<size_t>
SumData::get_report_step_from_time(time_t sim_time) const {
    if (!this->check_sim_time(sim_time))
        return std::nullopt;
    else {
        auto files = this->index.lookup_time(sim_time);
        if (files.first != files.second)
            return std::nullopt;

        return this->file(*files.first).report_step_from_time(sim_time);
    }
}

double SumData::get_from_sim_days(double sim_days,
                                  const rd::smspec_node &smspec_node) const {
    time_t sim_time = rd_smspec_get_start_time(this->smspec);
    util_inplace_forward_days_utc(&sim_time, sim_days);
    return this->get_from_sim_time(sim_time, smspec_node);
}

time_t SumData::iget_sim_time(size_t ministep_index) const {
    const auto loc = this->locate(ministep_index);
    return loc.file.iget_sim_time(loc.local_index);
}

time_t SumData::get_report_time(size_t report_step) const {
    if (report_step == 0)
        return rd_smspec_get_start_time(this->smspec);
    else {
        size_t internal_index = this->iget_report_end(report_step);
        return this->iget_sim_time(internal_index);
    }
}

size_t SumData::get_first_report_step() const {
    const auto &data_file = this->data_files[0];
    return data_file->first_report();
}

size_t SumData::get_last_report_step() const {
    const auto &data_file = this->data_files.back();
    return data_file->last_report();
}

std::vector<time_t> SumData::alloc_time_vector() const {
    std::vector<time_t> output_data(this->get_length());

    size_t offset = 0;
    for (const auto &index_node : this->index) {
        this->file(index_node)
            .get_time(index_node.length, &output_data[offset]);
        offset += index_node.length;
    }
    return output_data;
}

std::vector<time_t> SumData::alloc_report_time_vector() const {
    std::vector<time_t> output_data;
    for (const auto &index_node : this->index)
        this->file(index_node).get_time_report(index_node.length, output_data);
    return output_data;
}

void SumData::init_double_vector(size_t params_index,
                                 double *output_data) const {
    if (params_index >= rd_smspec_get_params_size(this->smspec))
        throw std::out_of_range("Out of range");

    const double default_value = this->default_value(params_index);

    size_t offset = 0;
    for (const auto &index_node : this->index) {
        auto &data_file = this->file(index_node);
        const std::optional<size_t> file_params_index =
            index_node.params_map[params_index];

        if (file_params_index)
            data_file.get_data(*file_params_index, index_node.length,
                               &output_data[offset]);
        else
            std::fill_n(&output_data[offset], index_node.length, default_value);
        offset += index_node.length;
    }
}

std::vector<double> SumData::alloc_data_vector(size_t params_index) const {
    std::vector<double> output_data(this->get_length());
    this->init_double_vector(params_index, output_data.data());
    return output_data;
}

std::vector<double>
SumData::alloc_report_data_vector(size_t params_index) const {
    if (params_index >= rd_smspec_get_params_size(this->smspec))
        throw std::out_of_range("Out of range");

    const double default_value = this->default_value(params_index);

    std::vector<double> output_data;
    for (const auto &index_node : this->index)
        this->file(index_node)
            .get_data_report(index_node.params_map[params_index],
                             index_node.length, default_value, output_data);
    return output_data;
}

void SumData::init_double_vector_interp(const rd::smspec_node &smspec_node,
                                        const std::vector<time_t> &time_points,
                                        double *output_data) const {
    bool is_rate = smspec_node.is_rate();
    size_t params_index = smspec_node.get_params_index();
    time_t start_time = this->get_data_start();
    time_t end_time = this->get_sim_end();
    double start_value = 0;
    double end_value = 0;

    if (!is_rate) {
        start_value = this->iget_first_value(params_index);
        end_value = this->iget_last_value(params_index);
    }

    for (size_t time_index = 0; time_index < time_points.size(); time_index++) {
        time_t sim_time = time_points[time_index];
        double value;
        if (sim_time < start_time)
            value = start_value;

        else if (sim_time > end_time)
            value = end_value;

        else {
            value = this->interp_get(this->interp_from_sim_time(sim_time),
                                     params_index, is_rate);
        }

        output_data[time_index] = value;
    }
}

void SumData::init_double_frame_interp(const rd_sum_vector_type *keywords,
                                       const std::vector<time_t> &time_points,
                                       double *output_data) const {
    const size_t num_keywords = rd_sum_vector_get_size(keywords);
    const time_t start_time = this->get_data_start();
    const time_t end_time = this->get_sim_end();
    const size_t length = this->get_length();
    const auto [params_indices, key_indices] = present_keywords(keywords);
    std::vector<bool> is_rate(key_indices.size());
    for (size_t j = 0; j < key_indices.size(); j++)
        is_rate[j] = rd_sum_vector_iget_is_rate(keywords, key_indices[j]);

    RowCache prev_row, next_row;

    for (size_t time_index = 0; time_index < time_points.size(); time_index++) {
        const time_t sim_time = time_points[time_index];
        double *output_row = output_data + time_index * num_keywords;
        std::fill(output_row, output_row + num_keywords, std::nan(""));
        if (key_indices.empty())
            continue;

        if (sim_time < start_time) {
            prev_row.load(*this, 0, params_indices);
            for (size_t j = 0; j < key_indices.size(); j++)
                output_row[key_indices[j]] = is_rate[j] ? 0 : prev_row[j];
        } else if (sim_time > end_time) {
            if (length == 0)
                throw std::out_of_range(
                    "SumData::init_double_frame_interp: no data");
            next_row.load(*this, length - 1, params_indices);
            for (size_t j = 0; j < key_indices.size(); j++)
                output_row[key_indices[j]] = is_rate[j] ? 0 : next_row[j];
        } else {
            const Interp interp = this->interp_from_sim_time(sim_time);
            if (next_row.time_index == interp.index1 &&
                prev_row.time_index != interp.index1)
                std::swap(prev_row, next_row);
            prev_row.load(*this, interp.index1, params_indices);
            next_row.load(*this, interp.index2, params_indices);

            for (size_t j = 0; j < key_indices.size(); j++)
                output_row[key_indices[j]] =
                    interp.interpolate(prev_row[j], next_row[j], is_rate[j]);
        }
    }
}

void SumData::init_double_frame(const rd_sum_vector_type *keywords,
                                double *output_data) const {
    const size_t num_keywords = rd_sum_vector_get_size(keywords);
    const size_t length = this->get_length();
    const auto [params_indices, key_indices] = present_keywords(keywords);
    std::vector<double> values(params_indices.size());

    for (size_t time_index = 0; time_index < length; time_index++) {
        double *output_row = output_data + time_index * num_keywords;
        std::fill(output_row, output_row + num_keywords, std::nan(""));
        this->iget_row(time_index, params_indices, values.data());
        for (size_t j = 0; j < key_indices.size(); j++)
            output_row[key_indices[j]] = values[j];
    }
}

/** This function will return the total number of ministeps in the
    current rd_sum_data instance; but observe that actual series of
    ministeps can have non-zero offset and also "holes" in the series. */
size_t SumData::get_length() const { return this->index.length(); }

double SumData::iget_last_value(size_t param_index) const {
    const size_t length = this->get_length();
    if (length == 0)
        throw std::out_of_range("SumData::iget_last_value: no data");
    return this->iget(length - 1, param_index);
}

double SumData::iget_first_value(size_t param_index) const {
    return this->iget(0, param_index);
}
