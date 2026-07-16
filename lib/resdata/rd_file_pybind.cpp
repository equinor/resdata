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
    m.def("_writable", [](py::handle self) {
        return from_cwrap<rd::File>(self)->is_writable();
    });
    m.def("_save_kw", [](py::handle self, py::handle kw) {
        from_cwrap<rd::File>(self)->save_kw(from_cwrap<rd_kw_type>(kw));
    });
    m.def("_close",
          [](py::handle self) { from_cwrap<rd::File>(self)->close(); });
    m.def("_free", [](py::handle self) { delete from_cwrap<rd::File>(self); });
    m.def("_iget_restart_time", [](py::handle self, int index) {
        return static_cast<std::int64_t>(
            from_cwrap<rd::File>(self)->restart_sim_date(index));
    });
    m.def("_iget_restart_days", [](py::handle self, int index) {
        return from_cwrap<rd::File>(self)->restart_sim_days(index);
    });
    m.def("_get_restart_index", [](py::handle self, std::int64_t sim_time) {
        return from_cwrap<rd::File>(self)->find_sim_time(
            static_cast<time_t>(sim_time));
    });
    m.def("_get_src_file", [](py::handle self) {
        return from_cwrap<rd::File>(self)->filename();
    });
    m.def("_fwrite", [](py::handle self, ERT::FortIO &fortio) {
        from_cwrap<rd::File>(self)->write(fortio, 0);
    });
    m.def("_has_report_step", [](py::handle self, int report_step) {
        return from_cwrap<rd::File>(self)->has_report_step(report_step);
    });
    m.def("_has_sim_time", [](py::handle self, std::int64_t sim_time) {
        return from_cwrap<rd::File>(self)->has_sim_time(
            static_cast<time_t>(sim_time));
    });
    m.def("_get_global_view", [](py::handle self) {
        return from_cwrap<rd::File>(self)->get_global_view();
    });
    m.def("_write_index", [](py::handle self, std::string index_filename) {
        return from_cwrap<rd::File>(self)->write_index(index_filename);
    });
}
} // namespace
