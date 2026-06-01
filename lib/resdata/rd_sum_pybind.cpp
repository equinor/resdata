#include <cstdint>
#include <ctime>
#include <optional>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <fmt/format.h>

#include <ert/util/double_vector.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>

#include <resdata/rd_sum.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/rd_sum_vector.hpp>
#include <resdata/smspec_node.hpp>

namespace py = pybind11;

namespace {
template <typename T> T *from_cwrap(py::handle obj) {
    if (obj.is_none())
        return nullptr;

    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());

    return reinterpret_cast<T *>(pointer);
}

PYBIND11_MODULE(_rd_sum, m) {
    m.doc() = "pybind11 bindings between rd_sum.py and rd_sum.cpp";

    m.def(
        "_fread_alloc_case",
        [](std::string load_case, std::string key_join_string,
           bool include_restart, bool lazy_load, int file_options) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_fread_alloc_case(
                load_case.c_str(), key_join_string.c_str(), include_restart,
                lazy_load, file_options));
        },
        py::return_value_policy::reference);
    m.def(
        "_fread_alloc",
        [](std::string header_file, py::handle data_files,
           std::string key_join_string, bool include_restart, bool lazy_load,
           int file_options) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_fread_alloc(
                header_file.c_str(), from_cwrap<stringlist_type>(data_files),
                key_join_string.c_str(), include_restart, lazy_load,
                file_options));
        },
        py::return_value_policy::reference);
    m.def(
        "_create_writer",
        [](std::string rd_case, bool fmt_output, bool unified,
           std::string key_join_string, std::time_t sim_start,
           bool time_in_days, int nx, int ny, int nz,
           std::optional<std::string> restart_case, int restart_step) {
            return reinterpret_cast<std::uintptr_t>(
                make_summary_writer(rd_case, fmt_output, unified,
                                    key_join_string, sim_start, time_in_days,
                                    nx, ny, nz, restart_case, restart_step)
                    .release());
        },
        py::return_value_policy::reference);
    m.def(
        "_resample",
        [](py::handle self, std::string new_case, py::handle times,
           bool lower_extrapolation, bool upper_extrapolation) {
            return reinterpret_cast<std::uintptr_t>(
                rd_sum_alloc_resample(from_cwrap<rd_sum_type>(self),
                                      new_case.c_str(),
                                      from_cwrap<time_t_vector_type>(times),
                                      lower_extrapolation, upper_extrapolation)
                    .release());
        },
        py::return_value_policy::reference);
    m.def("_free",
          [](py::handle self) { rd_sum_free(from_cwrap<rd_sum_type>(self)); });
    m.def("_iiget", [](py::handle self, int time_index, int param_index) {
        return rd_sum_iget(from_cwrap<rd_sum_type>(self), time_index,
                           param_index);
    });
    m.def("_data_length", [](py::handle self) {
        return rd_sum_get_data_length(from_cwrap<rd_sum_type>(self));
    });
    m.def("_iget_sim_days", [](py::handle self, int index) {
        return rd_sum_iget_sim_days(from_cwrap<rd_sum_type>(self), index);
    });
    m.def("_iget_report_step", [](py::handle self, int index) {
        return rd_sum_iget_report_step(from_cwrap<rd_sum_type>(self), index);
    });
    m.def("_iget_sim_time", [](py::handle self, int index) -> std::time_t {
        return rd_sum_iget_sim_time(from_cwrap<rd_sum_type>(self), index);
    });
    m.def("_get_report_end", [](py::handle self, int report_step) {
        return rd_sum_iget_report_end(from_cwrap<rd_sum_type>(self),
                                      report_step);
    });
    m.def("_get_general_var",
          [](py::handle self, int time_index, std::string lookup_kw) {
              return rd_sum_get_general_var(from_cwrap<rd_sum_type>(self),
                                            time_index, lookup_kw.c_str());
          });
    m.def("_get_general_var_index", [](py::handle self, std::string lookup_kw) {
        return rd_sum_get_general_var_params_index(
            from_cwrap<rd_sum_type>(self), lookup_kw.c_str());
    });
    m.def("_get_general_var_from_sim_days",
          [](py::handle self, double sim_days, std::string var) {
              return rd_sum_get_general_var_from_sim_days(
                  from_cwrap<rd_sum_type>(self), sim_days, var.c_str());
          });
    m.def("_get_general_var_from_sim_time",
          [](py::handle self, std::time_t sim_time, std::string var) {
              return rd_sum_get_general_var_from_sim_time(
                  from_cwrap<rd_sum_type>(self), sim_time, var.c_str());
          });
    m.def("_get_first_gt", [](py::handle self, int param_index, double limit) {
        return rd_sum_get_first_gt(from_cwrap<rd_sum_type>(self), param_index,
                                   limit);
    });
    m.def("_get_first_lt", [](py::handle self, int param_index, double limit) {
        return rd_sum_get_first_lt(from_cwrap<rd_sum_type>(self), param_index,
                                   limit);
    });
    m.def("_get_start_date", [](py::handle self) -> std::time_t {
        return rd_sum_get_start_time(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_end_date", [](py::handle self) -> std::time_t {
        return rd_sum_get_end_time(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_data_start", [](py::handle self) -> std::time_t {
        return rd_sum_get_data_start(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_last_report_step", [](py::handle self) {
        return rd_sum_get_last_report_step(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_first_report_step", [](py::handle self) {
        return rd_sum_get_first_report_step(from_cwrap<rd_sum_type>(self));
    });
    m.def("_has_key", [](py::handle self, std::string lookup_kw) {
        return rd_sum_has_general_var(from_cwrap<rd_sum_type>(self),
                                      lookup_kw.c_str());
    });
    m.def("_check_sim_time", [](py::handle self, std::time_t sim_time) {
        return rd_sum_check_sim_time(from_cwrap<rd_sum_type>(self), sim_time);
    });
    m.def("_check_sim_days", [](py::handle self, double sim_days) {
        return rd_sum_check_sim_days(from_cwrap<rd_sum_type>(self), sim_days);
    });
    m.def("_sim_length", [](py::handle self) {
        return rd_sum_get_sim_length(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_first_day", [](py::handle self) {
        return rd_sum_get_first_day(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_unit", [](py::handle self, std::string gen_key) -> std::string {
        return rd_sum_get_unit(from_cwrap<rd_sum_type>(self), gen_key.c_str());
    });
    m.def("_get_restart_step", [](py::handle self) {
        return rd_sum_get_restart_step(from_cwrap<rd_sum_type>(self));
    });
    m.def(
        "_get_restart_case",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_sum_get_restart_case(from_cwrap<rd_sum_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_get_simcase", [](py::handle self) -> std::string {
        return rd_sum_get_case(from_cwrap<rd_sum_type>(self));
    });
    m.def("_get_unit_system", [](py::handle self) {
        return static_cast<int>(
            rd_sum_get_unit_system(from_cwrap<rd_sum_type>(self)));
    });
    m.def("_get_base", [](py::handle self) -> std::optional<std::string> {
        const char *base = rd_sum_get_base(from_cwrap<rd_sum_type>(self));
        if (base == nullptr)
            return std::nullopt;
        return std::string(base);
    });
    m.def("_get_path", [](py::handle self) -> std::optional<std::string> {
        const char *path = rd_sum_get_path(from_cwrap<rd_sum_type>(self));
        if (path == nullptr)
            return std::nullopt;
        return std::string(path);
    });
    m.def("_get_abs_path", [](py::handle self) -> std::optional<std::string> {
        const char *path = rd_sum_get_abs_path(from_cwrap<rd_sum_type>(self));
        if (path == nullptr)
            return std::nullopt;
        return std::string(path);
    });
    m.def("_get_report_step_from_time",
          [](py::handle self, std::time_t sim_time) {
              return rd_sum_get_report_step_from_time(
                  from_cwrap<rd_sum_type>(self), sim_time);
          });
    m.def("_get_report_step_from_days", [](py::handle self, double sim_days) {
        return rd_sum_get_report_step_from_days(from_cwrap<rd_sum_type>(self),
                                                sim_days);
    });
    m.def("_get_report_time",
          [](py::handle self, int report_step) -> std::time_t {
              return rd_sum_get_report_time(from_cwrap<rd_sum_type>(self),
                                            report_step);
          });
    m.def("_fwrite_sum", [](py::handle self) {
        rd_sum_fwrite(from_cwrap<rd_sum_type>(self));
    });
    m.def("_can_write", [](py::handle self) {
        return rd_sum_can_write(from_cwrap<rd_sum_type>(self));
    });
    m.def("_set_case", [](py::handle self, std::string input_arg) {
        rd_sum_set_case(from_cwrap<rd_sum_type>(self), input_arg);
    });
    m.def(
        "_alloc_time_vector",
        [](py::handle self, bool report_only) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_alloc_time_vector(
                from_cwrap<rd_sum_type>(self), report_only));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_data_vector",
        [](py::handle self, int data_index, bool report_only) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_alloc_data_vector(
                from_cwrap<rd_sum_type>(self), data_index, report_only));
        },
        py::return_value_policy::reference);
    m.def(
        "_get_var_node",
        [](py::handle self, std::string lookup_kw) {
            return reinterpret_cast<std::uintptr_t>(
                const_cast<rd::smspec_node *>(rd_sum_get_general_var_node(
                    from_cwrap<rd_sum_type>(self), lookup_kw.c_str())));
        },
        py::return_value_policy::reference);
    m.def(
        "_create_well_list",
        [](py::handle self, std::optional<std::string> pattern) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_alloc_well_list(
                from_cwrap<rd_sum_type>(self),
                pattern.has_value() ? pattern->c_str() : nullptr));
        },
        py::return_value_policy::reference);
    m.def(
        "_create_group_list",
        [](py::handle self, std::optional<std::string> pattern) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_alloc_group_list(
                from_cwrap<rd_sum_type>(self),
                pattern.has_value() ? pattern->c_str() : nullptr));
        },
        py::return_value_policy::reference);
    m.def("_select_matching_keys",
          [](py::handle self, std::optional<std::string> pattern,
             py::handle keys) {
              rd_sum_select_matching_general_var_list(
                  from_cwrap<rd_sum_type>(self),
                  pattern.has_value() ? pattern->c_str() : nullptr,
                  from_cwrap<stringlist_type>(keys));
          });

    m.def(
        "_solve_days",
        [](py::handle self, std::string gen_key, double cmp_value,
           bool rates_clamp_lower) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_alloc_days_solution(
                from_cwrap<rd_sum_type>(self), gen_key.c_str(), cmp_value,
                rates_clamp_lower));
        },
        py::return_value_policy::reference);
    m.def(
        "_solve_dates",
        [](py::handle self, std::string gen_key, double cmp_value,
           bool rates_clamp_lower) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_alloc_time_solution(
                from_cwrap<rd_sum_type>(self), gen_key.c_str(), cmp_value,
                rates_clamp_lower));
        },
        py::return_value_policy::reference);

    m.def(
        "_add_variable",
        [](py::handle self, std::string keyword,
           std::optional<std::string> wgname, int num, std::string unit,
           double default_value) {
            return reinterpret_cast<std::uintptr_t>(
                const_cast<rd::smspec_node *>(rd_sum_add_var(
                    from_cwrap<rd_sum_type>(self), keyword.c_str(),
                    wgname.has_value() ? wgname->c_str() : nullptr, num,
                    unit.c_str(), default_value)));
        },
        py::return_value_policy::reference);
    m.def(
        "_add_local_variable",
        [](py::handle self, std::string keyword,
           std::optional<std::string> wgname, int num, std::string unit,
           std::string lgr, int lgr_i, int lgr_j, int lgr_k,
           double default_value) {
            return reinterpret_cast<std::uintptr_t>(
                const_cast<rd::smspec_node *>(rd_sum_add_local_var(
                    from_cwrap<rd_sum_type>(self), keyword.c_str(),
                    wgname.has_value() ? wgname->c_str() : nullptr, num,
                    unit.c_str(), lgr.c_str(), lgr_i, lgr_j, lgr_k,
                    default_value)));
        },
        py::return_value_policy::reference);
    m.def(
        "_add_tstep",
        [](py::handle self, int report_step, double sim_seconds) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_add_tstep(
                from_cwrap<rd_sum_type>(self), report_step, sim_seconds));
        },
        py::return_value_policy::reference);

    m.def("_export_csv",
          [](py::handle self, std::string filename, py::handle var_list,
             std::string date_format, std::string sep) {
              rd_sum_export_csv(from_cwrap<rd_sum_type>(self), filename.c_str(),
                                from_cwrap<stringlist_type>(var_list),
                                date_format.c_str(), sep.c_str());
          });
    m.def("_dump_csv_line", [](py::handle self, std::time_t sim_time,
                               py::handle key_words, py::handle file) {
        rd_sum_fwrite_interp_csv_line(from_cwrap<rd_sum_type>(self), sim_time,
                                      from_cwrap<rd_sum_vector_type>(key_words),
                                      from_cwrap<FILE>(file));
    });
    m.def("_get_interp_vector", [](py::handle self, std::time_t sim_time,
                                   py::handle key_words, py::handle data) {
        rd_sum_get_interp_vector(from_cwrap<rd_sum_type>(self), sim_time,
                                 from_cwrap<rd_sum_vector_type>(key_words),
                                 from_cwrap<double_vector_type>(data));
    });
    m.def("_init_pandas_frame",
          [](py::handle self, py::handle keywords,
             py::array_t<double, py::array::c_style | py::array::forcecast>
                 data) {
              auto keyvec = from_cwrap<rd_sum_vector_type>(keywords);
              auto rd_sum = from_cwrap<rd_sum_type>(self);
              double *out = data.mutable_data();
              int keylen = rd_sum_vector_get_size(keyvec);
              int timelen = rd_sum_get_data_length(rd_sum);
              if (data.request().size < timelen * keylen)
                  throw std::invalid_argument("Incorrect size of buffer");
              for (int time_index = 0; time_index < timelen; time_index++) {
                  for (int key_index = 0; key_index < keylen; key_index++) {
                      int param_index =
                          rd_sum_vector_iget_param_index(keyvec, key_index);
                      int data_index = key_index + time_index * keylen;
                      out[data_index] =
                          rd_sum_iget(rd_sum, time_index, param_index);
                  }
              }
          });
    m.def("_init_pandas_frame_interp",
          [](py::handle self, py::handle keywords, py::handle time_points,
             py::array_t<double, py::array::c_style | py::array::forcecast>
                 data) {
              rd_sum_init_double_frame_interp(
                  from_cwrap<rd_sum_type>(self),
                  from_cwrap<rd_sum_vector_type>(keywords),
                  from_cwrap<time_t_vector_type>(time_points),
                  data.mutable_data());
          });
    m.def("_identify_var_type", [](std::string var) {
        return static_cast<int>(rd_sum_identify_var_type(var.c_str()));
    });
    m.def("_is_rate", [](std::string var) {
        return smspec_node_identify_rate(var.c_str());
    });
    m.def("_is_total", [](std::string var, int var_type) {
        return smspec_node_identify_total(
            var.c_str(), static_cast<rd_smspec_var_type>(var_type));
    });
    m.def("_get_last_value", [](py::handle self, std::string gen_key) {
        return rd_sum_get_last_value_gen_key(from_cwrap<rd_sum_type>(self),
                                             gen_key.c_str());
    });
    m.def("_get_first_value", [](py::handle self, std::string gen_key) {
        return rd_sum_get_first_value_gen_key(from_cwrap<rd_sum_type>(self),
                                              gen_key.c_str());
    });
    m.def("_init_numpy_vector",
          [](py::handle self, std::string gen_key,
             py::array_t<double, py::array::c_style | py::array::forcecast>
                 data) {
              rd_sum_init_double_vector(from_cwrap<rd_sum_type>(self),
                                        gen_key.c_str(), data.mutable_data());
          });
    m.def("_init_numpy_vector_interp",
          [](py::handle self, std::string gen_key, py::handle time_points,
             py::array_t<double, py::array::c_style | py::array::forcecast>
                 data) {
              rd_sum_init_double_vector_interp(
                  from_cwrap<rd_sum_type>(self), gen_key.c_str(),
                  from_cwrap<time_t_vector_type>(time_points),
                  data.mutable_data());
          });
    m.def(
        "_init_numpy_datetime64",
        [](py::handle self,
           py::array_t<int64_t, py::array::c_style | py::array::forcecast> data,
           int multiplier) {
            int64_t *out = data.mutable_data();
            auto rd_sum = from_cwrap<rd_sum_type>(self);
            int len = rd_sum_get_data_length(rd_sum);
            if (data.request().size < len)
                throw std::invalid_argument("Incorrect size of buffer");
            for (int i = 0; i < len; i++)
                out[i] = rd_sum_iget_sim_time(rd_sum, i) * multiplier;
        });
}
} // namespace
