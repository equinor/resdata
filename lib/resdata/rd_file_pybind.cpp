#include <cstdint>
#include <cstdio>
#include <ctime>

#include <string>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_flag.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_file, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings between rd_file.py and rd_file.cpp";

    m.def("_open", [](std::string filename, FileMode flags) -> py::object {
        auto *file = rd::File::open(filename, flags).release();
        return py::cast(reinterpret_cast<std::uintptr_t>(file));
    });
    m.def(
        "_fast_open",
        [](std::string filename, std::string index_filename,
           FileMode flags) -> py::object {
            auto *file =
                rd::File::fast_open(filename, index_filename, flags).release();
            return py::cast(reinterpret_cast<std::uintptr_t>(file));
        });
    m.def("_get_file_type", [](std::string filename) {
        bool fmt_file = false;
        int report_step = 0;
        auto file_type =
            rd_get_file_type(filename.c_str(), &fmt_file, &report_step);
        return std::make_tuple(static_cast<int>(file_type), fmt_file,
                               report_step);
    });
    m.def("_writable", [](py::handle self) {
        return rd_file_writable(from_cwrap<rd_file_type>(self));
    });
    m.def("_save_kw", [](py::handle self, py::handle kw) {
        rd_file_save_kw(from_cwrap<rd_file_type>(self),
                        from_cwrap<rd_kw_type>(kw));
    });
    m.def("_close", [](py::handle self) {
        rd_file_close(from_cwrap<rd_file_type>(self));
    });
    m.def("_free",
          [](py::handle self) { delete from_cwrap<rd_file_type>(self); });
    m.def("_iget_restart_time", [](py::handle self, int index) {
        return static_cast<std::int64_t>(rd_file_iget_restart_sim_date(
            from_cwrap<rd_file_type>(self), index));
    });
    m.def("_iget_restart_days", [](py::handle self, int index) {
        return rd_file_iget_restart_sim_days(from_cwrap<rd_file_type>(self),
                                             index);
    });
    m.def("_get_restart_index", [](py::handle self, std::int64_t sim_time) {
        return rd_file_get_restart_index(from_cwrap<rd_file_type>(self),
                                         static_cast<time_t>(sim_time));
    });
    m.def("_get_src_file", [](py::handle self) {
        return rd_file_get_src_file(from_cwrap<rd_file_type>(self));
    });
    m.def("_fwrite", [](py::handle self, ERT::FortIO &fortio) {
        from_cwrap<rd::File>(self)->write(fortio, 0);
    });
    m.def("_has_report_step", [](py::handle self, int report_step) {
        return rd_file_has_report_step(from_cwrap<rd_file_type>(self),
                                       report_step);
    });
    m.def("_has_sim_time", [](py::handle self, std::int64_t sim_time) {
        return rd_file_has_sim_time(from_cwrap<rd_file_type>(self),
                                    static_cast<time_t>(sim_time));
    });
    m.def("_get_global_view", [](py::handle self) {
        return rd_file_get_global_view(from_cwrap<rd_file_type>(self));
    });
    m.def("_write_index", [](py::handle self, std::string index_filename) {
        return from_cwrap<rd::File>(self)->write_index(index_filename);
    });
}
} // namespace
