#include <cstdint>
#include <cstdio>

#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/well/well_info.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_well_info, m) {
    m.doc() = "pybind11 bindings between well_info.py and well_info.cpp";

    m.def("_alloc", [](py::handle grid) -> py::object {
        auto *well_info = well_info_alloc(from_cwrap<rd_grid_type>(grid));
        if (well_info == nullptr)
            return py::none();
        return py::cast(reinterpret_cast<std::uintptr_t>(well_info));
    });
    m.def("_free", [](py::handle self) {
        well_info_free(from_cwrap<well_info_type>(self));
    });
    m.def("_load_rstfile", [](py::handle self, std::string filename,
                              bool load_segment_information) {
        well_info_load_rstfile(from_cwrap<well_info_type>(self),
                               filename.c_str(), load_segment_information);
    });
    m.def("_load_rst_resfile", [](py::handle self, py::handle rst_file,
                                  bool load_segment_information) {
        well_info_load_rst_resfile(from_cwrap<well_info_type>(self),
                                   from_cwrap<rd_file_type>(rst_file),
                                   load_segment_information);
    });
    m.def("_get_well_count", [](py::handle self) {
        return well_info_get_num_wells(from_cwrap<well_info_type>(self));
    });
    m.def("_iget_well_name", [](py::handle self, int well_index) {
        return well_info_iget_well_name(from_cwrap<well_info_type>(self),
                                        well_index);
    });
    m.def("_has_well", [](py::handle self, std::string well_name) {
        return well_info_has_well(from_cwrap<well_info_type>(self),
                                  well_name.c_str());
    });
    m.def(
        "_get_ts",
        [](py::handle self, std::string well_name) {
            return reinterpret_cast<std::uintptr_t>(well_info_get_ts(
                from_cwrap<well_info_type>(self), well_name.c_str()));
        },
        py::return_value_policy::reference);
}
} // namespace
