#include <cstdint>
#include <string>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_util.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_rd_util, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResdataUtil";

    m.def("_get_num_cpu", [](std::string data_file) {
        return rd_get_num_cpu(data_file.c_str());
    });
    m.def("_get_file_type", [](std::string filename) {
        bool fmt = false;
        int step = -1;
        rd_file_enum file_type =
            rd_get_file_type(filename.c_str(), &fmt, &step);
        return std::make_tuple(static_cast<int>(file_type), fmt, step);
    });
    m.def("_get_start_date", [](std::string data_file) {
        return static_cast<std::int64_t>(rd_get_start_date(data_file.c_str()));
    });
    m.def("_get_report_step", [](std::string filename) {
        return rd_filename_report_nr(filename.c_str());
    });
}
