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
#include <resdata/rd_grav.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_grav, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for rd_grav.cpp";
    m.def("_free", [](py::handle self) {
        auto *grav = from_cwrap<rd_grav_type>(self);
        rd_grav_free(grav);
    });

    m.def("_alloc", [](py::handle grid, py::handle init_file) -> py::object {
        auto *grid_ptr = from_cwrap<rd_grid_type>(grid);
        auto *init_file_ptr = from_cwrap<rd_file_type>(init_file);

        auto *grav = rd_grav_alloc(grid_ptr, init_file_ptr);

        if (grav == nullptr) {
            return py::none();
        }

        return py::int_(reinterpret_cast<uintptr_t>(grav));
    });

    m.def("_add_survey_RPORV",
          [](py::handle self, const std::string &name,
             rd::FileView *restart_file) -> py::object {
              auto *grav = from_cwrap<rd_grav_type>(self);
              auto *survey = rd_grav_add_survey_RPORV(grav, name, restart_file);
              if (survey == nullptr)
                  return py::none();
              return py::int_(reinterpret_cast<uintptr_t>(survey));
          });

    m.def("_add_survey_PORMOD", [](py::handle self, const std::string &name,
                                   rd::FileView *restart_file) {
        auto *grav = from_cwrap<rd_grav_type>(self);
        rd_grav_add_survey_PORMOD(grav, name, restart_file);
    });

    m.def("_add_survey_FIP",
          [](py::handle self, const std::string &name,
             rd::FileView *restart_file) -> py::object {
              auto *grav = from_cwrap<rd_grav_type>(self);
              auto *survey = rd_grav_add_survey_FIP(grav, name, restart_file);

              if (survey == nullptr) {
                  return py::none();
              }

              return py::int_(reinterpret_cast<uintptr_t>(survey));
          });

    m.def("_add_survey_RFIP", [](py::handle self, const std::string &name,
                                 rd::FileView *restart_file) {
        auto *grav = from_cwrap<rd_grav_type>(self);
        rd_grav_add_survey_RFIP(grav, name, restart_file);
    });

    m.def("_eval",
          [](py::handle self, const std::string &base,
             const std::optional<std::string> &monitor, py::handle region,
             double utm_x, double utm_y, double depth, int phase_mask) {
              auto *grav = from_cwrap<rd_grav_type>(self);

              rd_region_type *region_ptr = nullptr;
              if (!region.is_none()) {
                  region_ptr = from_cwrap<rd_region_type>(region);
              }

              return rd_grav_eval(grav, base, monitor, region_ptr, utm_x, utm_y,
                                  depth, phase_mask);
          });

    m.def("_new_std_density",
          [](py::handle self, int phase, double default_density) {
              auto *grav = from_cwrap<rd_grav_type>(self);
              rd_grav_new_std_density(grav, static_cast<rd_phase_enum>(phase),
                                      default_density);
          });

    m.def("_add_std_density",
          [](py::handle self, int phase, int pvtnum, double density) {
              auto *grav = from_cwrap<rd_grav_type>(self);

              rd_grav_add_std_density(grav, static_cast<rd_phase_enum>(phase),
                                      pvtnum, density);
          });
}
