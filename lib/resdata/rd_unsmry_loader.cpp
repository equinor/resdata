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
      time_index(rd_smspec_get_time_index(smspec)),
      time_seconds(rd_smspec_get_time_seconds(smspec)),
      sim_start(rd_smspec_get_start_time(smspec)) {
    {
        rd_file_ptr file = rd::File::open(filename, file_options);
        this->file = std::move(file);
    }
    if (!rd_file_has_kw(this->file.get(), PARAMS_KW)) {
        throw std::bad_alloc();
    }

    if (rd_file_get_num_named_kw(this->file.get(), PARAMS_KW) !=
        rd_file_get_num_named_kw(this->file.get(), MINISTEP_KW)) {
        throw std::bad_alloc();
    }

    this->date_index = {{rd_smspec_get_date_day_index(smspec),
                         rd_smspec_get_date_month_index(smspec),
                         rd_smspec_get_date_year_index(smspec)}};
    auto file_view = rd_file_get_global_view(this->file.get());
    int length = file_view->num_named_kw(PARAMS_KW);

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

int unsmry_loader::length() const { return this->m_length; }

std::vector<double> unsmry_loader::get_vector(int pos) const {
    if (pos >= size)
        throw std::out_of_range(
            "unsmry_loader::get_vector pos: " + std::to_string(pos) +
            " PARAMS_SIZE: " + std::to_string(size));

    std::vector<double> data(this->length());
    auto index_map = make_int_vector(1, pos);
    float value;

    for (int index = 0; index < this->length(); index++) {
        file_view->index_fload_kw(PARAMS_KW, index, index_map.get(),
                                  (char *)&value);
        data[index] = value;
    }

    if (file_view->has_flags(FileMode::CLOSE_STREAM))
        file_view->close();

    return data;
}

// This is horribly inefficient
double unsmry_loader::iget(int time_index, int params_index) const {
    auto index_map = make_int_vector(1, params_index);
    float value;
    file_view->index_fload_kw(PARAMS_KW, time_index, index_map.get(),
                              (char *)&value);
    return value;
}

time_t unsmry_loader::iget_sim_time(int time_index) const {
    if (this->time_index >= 0) {
        double sim_seconds = this->iget_sim_seconds(time_index);
        time_t sim_time = this->sim_start;
        util_inplace_forward_seconds_utc(&sim_time, sim_seconds);
        return sim_time;
    } else {
        auto index_map = make_int_vector(3, 0);
        int_vector_iset(index_map.get(), 0, this->date_index[0]);
        int_vector_iset(index_map.get(), 1, this->date_index[1]);
        int_vector_iset(index_map.get(), 2, this->date_index[2]);

        float values[3];
        file_view->index_fload_kw(PARAMS_KW, time_index, index_map.get(),
                                  (char *)&values);

        return rd_make_date(util_roundf(values[0]), util_roundf(values[1]),
                            util_roundf(values[2]));
    }
}

double unsmry_loader::iget_sim_seconds(int time_index) const {
    if (this->time_index >= 0) {
        double raw_time = this->iget(time_index, this->time_index);
        return raw_time * this->time_seconds;
    } else {
        time_t sim_time = this->iget_sim_time(time_index);
        return util_difftime_seconds(this->sim_start, sim_time);
    }
}

std::vector<int> unsmry_loader::report_steps(int offset) const {
    std::vector<int> report_steps;
    int current_step = offset;
    for (const auto &file_kw : *file_view) {
        if (SEQHDR_KW == file_kw->get_header())
            current_step++;

        if (PARAMS_KW == file_kw->get_header())
            report_steps.push_back(current_step);
    }
    return report_steps;
}

std::vector<time_t> unsmry_loader::sim_time() const {
    if (this->time_index >= 0) {
        const std::vector<double> sim_seconds = this->sim_seconds();
        std::vector<time_t> st(this->length(), this->sim_start);

        for (size_t i = 0; i < st.size(); i++)
            util_inplace_forward_seconds_utc(&st[i], sim_seconds[i]);

        return st;

    } else {
        const auto day = this->get_vector(this->date_index[0]);
        const auto month = this->get_vector(this->date_index[1]);
        const auto year = this->get_vector(this->date_index[2]);
        std::vector<time_t> st(this->length());

        for (size_t i = 0; i < st.size(); i++)
            st[i] = rd_make_date(util_round(day[i]), util_round(month[i]),
                                 util_round(year[i]));

        return st;
    }
}

std::vector<double> unsmry_loader::sim_seconds() const {
    if (this->time_index >= 0) {
        std::vector<double> seconds = this->get_vector(this->time_index);
        for (size_t i = 0; i < seconds.size(); i++)
            seconds[i] *= this->time_seconds;

        return seconds;
    } else {
        std::vector<time_t> st = this->sim_time();
        std::vector<double> seconds(st.size());

        for (size_t i = 0; i < st.size(); i++)
            seconds[i] = util_difftime_seconds(this->sim_start, st[i]);

        return seconds;
    }
}

} // namespace rd
