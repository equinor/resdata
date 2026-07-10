#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_rsthead.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_rsthead, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for rd_rsthead.hpp";
    py::class_<RSTHead>(m, "ResdataRestartHead")
        .def(
            py::init([](std::optional<std::tuple<int, py::handle, py::handle,
                                                 std::optional<py::handle>>>
                            kw_arg,
                        std::optional<std::shared_ptr<rd::FileView>> rst_view) {
                if (kw_arg == std::nullopt && rst_view == std::nullopt)
                    throw std::invalid_argument(
                        "Cannot construct ResdataRestartHead without one of "
                        "kw_arg "
                        "and rst_view, both were None!");

                if (kw_arg.has_value()) {
                    return new RSTHead(
                        std::get<0>(*kw_arg),
                        from_cwrap<rd_kw_type>(std::get<1>(*kw_arg)),
                        from_cwrap<rd_kw_type>(std::get<2>(*kw_arg)),
                        from_cwrap<rd_kw_type>(std::get<3>(*kw_arg)));
                } else {
                    return std::make_unique<RSTHead>(
                               RSTHead::read(rst_view->get(), -1))
                        .release();
                }
            }),
            py::arg("kw_arg") = std::nullopt,
            py::arg("rst_view") = std::nullopt)
        .def("get_report_step", [](RSTHead &self) { return self.report_step; })
        .def("get_sim_date",
             [](RSTHead &self) {
                 py::object gmtime = py::module_::import("time").attr("gmtime");
                 py::object dt =
                     py::module_::import("datetime").attr("datetime");

                 return dt(*gmtime(self.sim_time)[py::slice(0, 6, 1)]);
             })
        .def("get_sim_days", [](RSTHead &self) { return self.sim_days; })
        .def("well_details", [](RSTHead &self) {
            py::dict res;
            res["NXCONZ"] = self.nxconz;
            res["NCWMAX"] = self.ncwmax;
            return res;
        });
}
} // namespace
