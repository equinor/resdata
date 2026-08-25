#include <ctime>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ert/util/int_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>

#include <detail/resdata/rd_unsmry_loader.hpp>

namespace rd {

unsmry_loader::unsmry_loader(const rd_smspec_type *smspec,
                             const std::string &filename, FileMode file_options)
    : size(rd_smspec_get_params_size(smspec)),
      time_info(rd_smspec_get_time_info(smspec)),
      sim_start(rd_smspec_get_start_time(smspec)) {
    {
        std::unique_ptr<rd::File> file = rd::File::open(filename, file_options);
        this->file = std::move(file);
    }
    if (!this->file->has_kw(PARAMS_KW)) {
        throw std::bad_alloc();
    }

    if (this->file->num_named_kw(PARAMS_KW) !=
        this->file->num_named_kw(MINISTEP_KW)) {
        throw std::bad_alloc();
    }

    auto file_view = this->file->get_global_view();
    size_t length = file_view->num_named_kw(PARAMS_KW);

    if (length > 0) {
        const rd_kw_type *params_kw = file_view->get_kw(PARAMS_KW, 0);
        if (params_kw == nullptr)
            throw std::invalid_argument(
                "Malformed summary file: missing PARAMS keyword entry");

        const rd_data_type params_data_type = rd_kw_get_data_type(params_kw);
        if (!rd_type_is_float(params_data_type))
            throw std::invalid_argument(
                "Malformed summary file: PARAMS keyword is not float");
    }

    this->file_view = file_view;
    this->m_length = length;
}

size_t unsmry_loader::length() const { return this->m_length; }

std::vector<double> unsmry_loader::get_vector(size_t pos) const {
    if (pos >= size)
        throw std::out_of_range(
            "unsmry_loader::get_vector pos: " + std::to_string(pos) +
            " PARAMS_SIZE: " + std::to_string(size));

    std::vector<double> data(this->length());
    std::vector<size_t> index_map{pos};
    float value;

    for (size_t index = 0; index < this->length(); index++) {
        file_view->index_fload_kw(PARAMS_KW, index, index_map, (char *)&value);
        data[index] = value;
    }

    if (file_view->has_flags(FileMode::CLOSE_STREAM))
        file_view->close();

    return data;
}

// This is horribly inefficient
double unsmry_loader::iget(size_t time_index, size_t params_index) const {
    std::vector<size_t> index_map{params_index};
    float value;
    file_view->index_fload_kw(PARAMS_KW, time_index, index_map, (char *)&value);
    return value;
}

std::vector<size_t> unsmry_loader::report_steps(size_t offset) const {
    std::vector<size_t> report_steps;
    size_t current_step = offset;
    for (const auto &file_kw : *file_view) {
        if (SEQHDR_KW == file_kw->get_header())
            current_step++;

        if (PARAMS_KW == file_kw->get_header()) {
            report_steps.push_back(current_step);
        }
    }
    return report_steps;
}

std::vector<time_t> unsmry_loader::sim_time() const {
    const auto *date = std::get_if<DateParamsIndex>(&this->time_info);
    if (date == nullptr) {
        const std::vector<double> sim_seconds = this->sim_seconds();
        std::vector<time_t> st(this->length(), this->sim_start);

        for (size_t i = 0; i < st.size(); i++)
            util_inplace_forward_seconds_utc(&st[i], sim_seconds[i]);

        return st;
    }

    const auto day = this->get_vector(date->day);
    const auto month = this->get_vector(date->month);
    const auto year = this->get_vector(date->year);
    std::vector<time_t> st(this->length());

    for (size_t i = 0; i < st.size(); i++)
        st[i] = rd_make_date(util_round(day[i]), util_round(month[i]),
                             util_round(year[i]));

    return st;
}

std::vector<double> unsmry_loader::sim_seconds() const {
    const auto *time = std::get_if<TimeParamsIndex>(&this->time_info);
    if (time == nullptr) {
        std::vector<time_t> st = this->sim_time();
        std::vector<double> seconds(st.size());

        for (size_t i = 0; i < st.size(); i++)
            seconds[i] = util_difftime_seconds(this->sim_start, st[i]);

        return seconds;
    }

    std::vector<double> seconds = this->get_vector(time->params_index);
    for (size_t i = 0; i < seconds.size(); i++)
        seconds[i] *= time->seconds_per_unit;

    return seconds;
}

} // namespace rd
