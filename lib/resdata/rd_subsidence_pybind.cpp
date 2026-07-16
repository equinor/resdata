#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>
#include <resdata/rd_subsidence.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_subsidence, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for rd_subsidence.cpp";

    m.def("_alloc", [](py::handle grid, rd::File *init_file) -> py::object {
        auto *subsidence =
            rd_subsidence_alloc(from_cwrap<rd_grid_type>(grid), init_file);

        if (subsidence == nullptr)
            return py::none();

        return py::cast(reinterpret_cast<std::uintptr_t>(subsidence));
    });

    m.def("_free", [](py::handle self) {
        rd_subsidence_free(from_cwrap<rd_subsidence_type>(self));
    });

    m.def("_add_survey_PRESSURE",
          [](py::handle self, std::string survey_name,
             std::shared_ptr<rd::FileView> restart_file_view) {
              rd_subsidence_add_survey_PRESSURE(
                  from_cwrap<rd_subsidence_type>(self), survey_name,
                  restart_file_view.get());
          });

    m.def("_eval", [](py::handle self, std::string base,
                      std::optional<std::string> monitor, py::object region,
                      double utm_x, double utm_y, double depth,
                      double compressibility, double poisson_ratio) {
        rd_region_type *region_ptr = nullptr;
        if (!region.is_none()) {
            region_ptr = from_cwrap<rd_region_type>(region);
        }

        return rd_subsidence_eval(from_cwrap<rd_subsidence_type>(self), base,
                                  monitor, region_ptr, utm_x, utm_y, depth,
                                  compressibility, poisson_ratio);
    });

    m.def("_eval_geertsma", [](py::handle self, std::string base,
                               std::optional<std::string> monitor,
                               py::object region, double utm_x, double utm_y,
                               double depth, double youngs_modulus,
                               double poisson_ratio, double seabed) {
        rd_region_type *region_ptr = nullptr;
        if (!region.is_none()) {
            region_ptr = from_cwrap<rd_region_type>(region);
        }

        return rd_subsidence_eval_geertsma(
            from_cwrap<rd_subsidence_type>(self), base, monitor, region_ptr,
            utm_x, utm_y, depth, youngs_modulus, poisson_ratio, seabed);
    });

    m.def("_eval_geertsma_rporv",
          [](py::handle self, std::string base,
             std::optional<std::string> monitor, py::object region,
             double utm_x, double utm_y, double depth, double youngs_modulus,
             double poisson_ratio, double seabed) {
              rd_region_type *region_ptr = nullptr;
              if (!region.is_none()) {
                  region_ptr = from_cwrap<rd_region_type>(region);
              }

              return rd_subsidence_eval_geertsma_rporv(
                  from_cwrap<rd_subsidence_type>(self), base, monitor,
                  region_ptr, utm_x, utm_y, depth, youngs_modulus,
                  poisson_ratio, seabed);
          });

    m.def("_has_survey", [](py::handle self, std::string survey_name) {
        return rd_subsidence_has_survey(from_cwrap<rd_subsidence_type>(self),
                                        survey_name);
    });
}
