#include <cstdint>
#include <ctime>
#include <optional>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
py::object create_kw_reference(rd_kw_type *kw, py::handle parent) {
    if (!kw)
        return py::none();
    return ResdataKW().attr("createCReference")(
        reinterpret_cast<std::uintptr_t>(kw), parent);
}

py::object create_view_reference(rd_file_view_type *view, py::handle parent) {
    if (!view)
        return py::none();
    return ResdataFileView().attr("createCReference")(
        reinterpret_cast<std::uintptr_t>(view), parent);
}
} // namespace

PYBIND11_MODULE(_file_view, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResdataFileView";

    m.def("_iget_kw", [](py::handle self, int index) {
        return create_kw_reference(
            rd_file_view_iget_kw(from_cwrap<rd_file_view_type>(self), index),
            self);
    });
    m.def("_iget_named_kw", [](py::handle self, std::string kw, int index) {
        return create_kw_reference(
            rd_file_view_iget_named_kw(from_cwrap<rd_file_view_type>(self),
                                       kw.c_str(), index),
            self);
    });
    m.def("_get_unique_kw", [](py::handle self, int index) {
        const char *kw = rd_file_view_iget_distinct_kw(
            from_cwrap<rd_file_view_type>(self), index);
        if (!kw)
            return std::string();
        return std::string(kw);
    });
    m.def("_get_size", [](py::handle self) {
        return rd_file_view_get_size(from_cwrap<rd_file_view_type>(self));
    });
    m.def("_get_num_named_kw", [](py::handle self, std::string kw) {
        return rd_file_view_get_num_named_kw(
            from_cwrap<rd_file_view_type>(self), kw.c_str());
    });
    m.def("_get_unique_size", [](py::handle self) {
        return rd_file_view_get_num_distinct_kw(
            from_cwrap<rd_file_view_type>(self));
    });
    m.def("_create_block_view", [](py::handle self, std::string kw, int index) {
        return create_view_reference(
            rd_file_view_add_blockview(from_cwrap<rd_file_view_type>(self),
                                       kw.c_str(), index),
            self);
    });
    m.def("_create_block_view2",
          [](py::handle self, std::optional<std::string> start_kw,
             std::optional<std::string> stop_kw, int index) {
              return create_view_reference(
                  rd_file_view_add_blockview2(
                      from_cwrap<rd_file_view_type>(self),
                      start_kw ? start_kw->c_str() : nullptr,
                      stop_kw ? stop_kw->c_str() : nullptr, index),
                  self);
          });
    m.def("_restart_view",
          [](py::handle self, int seqnum_index, int report_step,
             std::int64_t sim_time, double sim_days) {
              return create_view_reference(
                  rd_file_view_add_restart_view(
                      from_cwrap<rd_file_view_type>(self), seqnum_index,
                      report_step, static_cast<time_t>(sim_time), sim_days),
                  self);
          });
}
