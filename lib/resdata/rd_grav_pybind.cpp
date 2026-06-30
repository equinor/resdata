#include <cstdint>
#include <optional>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grav.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_rd_grav, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for ResdataGrav";

    m.def(
        "_alloc",
        [](py::handle grid, py::handle init_file) {
            return reinterpret_cast<std::uintptr_t>(rd_grav_alloc(
                from_cwrap<rd_grid_type>(grid),
                from_cwrap<rd_file_type>(init_file)));
        },
        py::return_value_policy::reference);

    m.def("_free",
          [](py::handle self) { rd_grav_free(from_cwrap<rd_grav_type>(self)); });

    m.def("_add_survey_RPORV",
          [](py::handle self, std::string survey_name,
             py::handle restart_file_view) -> py::object {
              rd_grav_survey_type *survey = rd_grav_add_survey_RPORV(
                  from_cwrap<rd_grav_type>(self), survey_name.c_str(),
                  from_cwrap<rd_file_view_type>(restart_file_view));
              if (!survey)
                  return py::none();
              return py::cast(reinterpret_cast<std::uintptr_t>(survey));
          });

    m.def("_add_survey_PORMOD",
          [](py::handle self, std::string survey_name,
             py::handle restart_file_view) -> py::object {
              rd_grav_survey_type *survey = rd_grav_add_survey_PORMOD(
                  from_cwrap<rd_grav_type>(self), survey_name.c_str(),
                  from_cwrap<rd_file_view_type>(restart_file_view));
              if (!survey)
                  return py::none();
              return py::cast(reinterpret_cast<std::uintptr_t>(survey));
          });

    m.def("_add_survey_FIP",
          [](py::handle self, std::string survey_name,
             py::handle restart_file_view) -> py::object {
              rd_grav_survey_type *survey = rd_grav_add_survey_FIP(
                  from_cwrap<rd_grav_type>(self), survey_name.c_str(),
                  from_cwrap<rd_file_view_type>(restart_file_view));
              if (!survey)
                  return py::none();
              return py::cast(reinterpret_cast<std::uintptr_t>(survey));
          });

    m.def("_add_survey_RFIP",
          [](py::handle self, std::string survey_name,
             py::handle restart_file_view) -> py::object {
              rd_grav_survey_type *survey = rd_grav_add_survey_RFIP(
                  from_cwrap<rd_grav_type>(self), survey_name.c_str(),
                  from_cwrap<rd_file_view_type>(restart_file_view));
              if (!survey)
                  return py::none();
              return py::cast(reinterpret_cast<std::uintptr_t>(survey));
          });

    m.def("_new_std_density",
          [](py::handle self, int phase, double default_density) {
              rd_grav_new_std_density(from_cwrap<rd_grav_type>(self),
                                      static_cast<rd_phase_enum>(phase),
                                      default_density);
          });

    m.def("_add_std_density",
          [](py::handle self, int phase, int pvtnum, double density) {
              rd_grav_add_std_density(from_cwrap<rd_grav_type>(self),
                                      static_cast<rd_phase_enum>(phase), pvtnum,
                                      density);
          });

    m.def("_eval", [](py::handle self, std::string base,
                      std::optional<std::string> monitor, py::object region,
                      double utm_x, double utm_y, double depth,
                      int phase_mask) {
        rd_region_type *region_ptr = nullptr;
        if (!region.is_none())
            region_ptr = from_cwrap<rd_region_type>(region);

        return rd_grav_eval(from_cwrap<rd_grav_type>(self), base.c_str(),
                            monitor ? monitor->c_str() : nullptr, region_ptr,
                            utm_x, utm_y, depth, phase_mask);
    });
}
