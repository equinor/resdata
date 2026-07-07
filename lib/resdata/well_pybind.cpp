#include <cstddef>

#include <fmt/core.h>
#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/pytypes.h>
#include <pyerrors.h>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include <memory>
#include <optional>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fmt/format.h>
#include <filesystem>

#include <resdata/well/well_segment.hpp>
#include <resdata/well/well_segment_collection.hpp>
#include <resdata/well/well_conn.hpp>
#include <resdata/well/well_const.hpp>
#include <resdata/well/well_info.hpp>
#include <resdata/well/well_state.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/well/well_ts.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace fs = std::filesystem;
namespace py = pybind11;

void load_rstfile(WellInfo *well_info, py::handle rst_file,
                  bool load_segment_information) {
    if (py::isinstance<py::str>(rst_file)) {
        auto rst_file_name = rst_file.cast<std::string>();
        if (!fs::is_regular_file(rst_file_name)) {
            py::object os_err = py::module_::import("builtins").attr("OSError");
            py::set_error(os_err, ("No such file " + rst_file_name).c_str());
            throw py::error_already_set();
        }
        well_info->load_rstfile(rst_file_name, load_segment_information);
    } else
        well_info->load_rstfile(from_cwrap<rd_file_type>(rst_file),
                                load_segment_information);
}

template <class... Ts> struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

struct WellIter {
    const WellInfo *well_info;
    size_t idx = 0;

    WellIter(const WellInfo *v) : well_info(v) {}

    std::shared_ptr<WellTimeLine> next() {
        if (idx >= well_info->num_wells())
            throw py::stop_iteration();
        return well_info->get_ts(well_info->get_well_name(idx++));
    }
};

namespace {
PYBIND11_MODULE(well, m) {
    register_exceptions(m);
    auto well_type_module = m.def_submodule("well_type_enum");
    py::enum_<WellType>(well_type_module, "WellType")
        .value("ZERO", WellType::ZERO)
        .value("PRODUCER", WellType::PRODUCER)
        .value("WATER_INJECTOR", WellType::WATER_INJECTOR)
        .value("GAS_INJECTOR", WellType::GAS_INJECTOR)
        .value("OIL_INJECTOR", WellType::OIL_INJECTOR)
        .export_values();
    m.attr("WellType") = well_type_module.attr("WellType");

    auto well_conn_dir_module =
        m.def_submodule("well_connection_direction_enum");
    py::enum_<WellConnDir>(well_conn_dir_module, "WellConnectionDirection")
        .value("well_conn_dirX", WellConnDir::X)
        .value("well_conn_dirY", WellConnDir::Y)
        .value("well_conn_dirZ", WellConnDir::Z)
        .value("well_conn_fracX", WellConnDir::fracX)
        .value("well_conn_fracY", WellConnDir::fracY)
        .export_values();
    m.attr("WellConnectionDirection") =
        well_conn_dir_module.attr("WellConnectionDirection");

    auto well_conn_module = m.def_submodule("well_connection");
    py::class_<WellConnection, std::shared_ptr<WellConnection>>(
        well_conn_module, "WellConnection")
        .def(py::init([]() -> std::shared_ptr<WellConnection> {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly");
            throw py::error_already_set();
        }))
        .def("isOpen", &WellConnection::is_open)
        .def("ijk",
             [](WellConnection &self) -> std::tuple<int, int, int> {
                 return {self.get_i(), self.get_j(), self.get_k()};
             })
        .def("direction", &WellConnection::get_dir)
        .def("segmentId", &WellConnection::get_segment_id)
        .def("isFractureConnection", &WellConnection::is_fracture_connection)
        .def("isMatrixConnection", &WellConnection::is_matrix_connection)
        .def("connectionFactor", &WellConnection::get_connection_factor)
        .def("__eq__", &WellConnection::operator==)
        .def("__hash__",
             [](const WellConnection &self) {
                 return py::hash(py::make_tuple(
                     self.get_i(), self.get_j(), self.get_k(),
                     static_cast<int>(self.get_dir()), self.is_open(),
                     self.get_segment_id(), self.is_matrix_connection(),
                     self.get_connection_factor(), self.get_oil_rate(),
                     self.get_gas_rate(), self.get_water_rate(),
                     self.get_volume_rate()));
             })
        .def("isMultiSegmentWell", &WellConnection::is_MSW)
        .def("__repr__",
             [](py::object py_self) {
                 auto &self = py_self.cast<WellConnection &>();
                 std::string ijk = py::str(py_self.attr("ijk")());
                 std::string frac =
                     self.is_fracture_connection() ? "fracture " : "";
                 std::string open = self.is_open() ? "open " : "shut ";
                 std::string msw = self.is_MSW() ? " (multi segment)" : "";
                 std::string direction = py::str(py_self.attr("direction")());
                 return fmt::format(
                     "WellConnection({} {}{}{}, rates = (O:{},G:{},W:{}), "
                     "direction = {})",
                     ijk, frac, open, msw, self.get_oil_rate(),
                     self.get_gas_rate(), self.get_water_rate(), direction);
             })
        .def("gasRate", &WellConnection::get_gas_rate,
             "The gas rate, as stored in the restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "Mscf/day (field) or cm3/hour (lab). Use :meth:`gasRateSI` to get "
             "the\n"
             "value converted to SI units.\n")
        .def("waterRate", &WellConnection::get_water_rate,
             "The water rate, as stored in the restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "stb/day (field) or cm3/hour (lab). Use :meth:`waterRateSI` to "
             "get the\n"
             "value converted to SI units.\n")
        .def("oilRate", &WellConnection::get_oil_rate,
             "The oil rate, as stored in the restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "stb/day (field) or cm3/hour (lab). Use :meth:`oilRateSI` to get "
             "the\n"
             "value converted to SI units.\n")
        .def("volumeRate", &WellConnection::get_volume_rate,
             "The volume rate, at reservoir conditions, as stored in the "
             "restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "stb/day (field) or cm3/hour (lab). Use :meth:`volumeRateSI` to "
             "get "
             "the\n"
             "value converted to SI units.\n")
        .def("gasRateSI", &WellConnection::get_gas_rate_si,
             "The gas rate converted to SI units (m3/s).\n"
             "\n"
             "This is the raw :meth:`gasRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n")
        .def("waterRateSI", &WellConnection::get_water_rate_si,
             "The water rate converted to SI units (m3/s).\n"
             "\n"
             "This is the raw :meth:`waterRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n")
        .def("oilRateSI", &WellConnection::get_oil_rate_si,
             "The oil rate converted to SI units (m3/s).\n"
             "\n"
             "This is the raw :meth:`oilRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n")
        .def("volumeRateSI", &WellConnection::get_volume_rate_si,
             "The volume rate, at reservoir conditions, converted to SI units "
             "(m3/s).\n"
             "\n"
             "This is the raw :meth:`volumeRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n");
    m.attr("WellConnection") = well_conn_module.attr("WellConnection");

    auto well_segment_module = m.def_submodule("well_segment");
    py::class_<WellSegment, std::shared_ptr<WellSegment>>(well_segment_module,
                                                          "WellSegment")
        .def(py::init([]() -> std::shared_ptr<WellSegment> {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly");
            throw py::error_already_set();
        }))
        .def("__repr__",
             [](WellSegment &self) {
                 return fmt::format(
                     "WellSegment({{Segment ID:{}   BranchID:{}  Length:{}}})",
                     self.get_id(), self.get_branch_id(), self.get_length());
             })
        .def("__str__",
             [](WellSegment &self) {
                 return fmt::format(
                     "{{Segment ID:{}   BranchID:{}  Length:{}}}",
                     self.get_id(), self.get_branch_id(), self.get_length());
             })
        .def("id", &WellSegment::get_id)
        .def("linkCount", &WellSegment::get_link_count)
        .def("branchId", &WellSegment::get_branch_id)
        .def("outletId", &WellSegment::get_outlet_id)
        .def("isActive", &WellSegment::is_active)
        .def("isMainStem", &WellSegment::is_main_stem)
        .def("isNearestWellHead", &WellSegment::is_nearest_wellhead)
        .def("depth", &WellSegment::get_depth)
        .def("__len__", &WellSegment::get_length)
        .def("length", &WellSegment::get_length)
        .def("totalLength", &WellSegment::get_total_length)
        .def("diameter", &WellSegment::get_diameter);
    m.attr("WellSegment") = well_segment_module.attr("WellSegment");

    auto well_state_module = m.def_submodule("well_state");
    py::class_<WellState, std::shared_ptr<WellState>>(well_state_module,
                                                      "WellState")
        .def(py::init([]() -> std::shared_ptr<WellState> {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly");
            throw py::error_already_set();
        }))
        .def("name", &WellState::get_name)
        .def("isOpen", &WellState::is_open)
        .def("wellHead",
             [](py::object py_self) -> py::object {
                 auto &self = py_self.cast<WellState &>();
                 auto wellhead = self.get_global_wellhead();
                 if (!wellhead)
                     return py::none();
                 return py::cast(wellhead);
             })
        .def("wellNumber", &WellState::get_well_nr)
        .def("reportNumber", &WellState::get_report_nr)
        .def("simulationTime",
             [](WellState &self) -> py::object {
                 return CTime()(self.get_sim_time());
             })
        .def("wellType", &WellState::get_type)
        .def("hasGlobalConnections", &WellState::has_global_connections)
        .def(
            "globalConnections",
            [](WellState &self)
                -> std::vector<std::shared_ptr<WellConnection>> {
                if (auto connections = self.get_global_connections())
                    return *connections;
                return {};
            },
            "The list of well connections for the global grid.\n"
            "\n"
            "Note: Constructs a new list of references to the well "
            "connections.\n")
        .def("__len__", &WellState::num_segments)
        .def("numSegments", &WellState::num_segments)
        .def(
            "segments",
            [](WellState &self) {
                auto *segments = self.get_segments();
                int size = well_segment_collection_get_size(segments);
                std::vector<std::shared_ptr<WellSegment>> result;
                result.reserve(size);
                for (int i = 0; i < size; i++) {
                    auto segment = well_segment_collection_iget(segments, i);
                    result.push_back(segment);
                }
                return result;
            },
            "The list of segments in the well.\n"
            "\n"
            "Note: Constructs a new list of references to the well "
            "segments.\n")
        .def(
            "__getitem__",
            [](py::object py_self, py::int_ index) {
                auto &self = py_self.cast<WellState &>();
                auto *segments = self.get_segments();
                int size = well_segment_collection_get_size(segments);
                if (index < py::int_(0))
                    index += py::int_(size);
                if (!((py::int_(0) <= index) && (index < py::int_(size))))
                    throw py::index_error(
                        fmt::format("Invalid index:{} - valid range [0,{})",
                                    index.cast<long>(), size));
                return well_segment_collection_iget(segments,
                                                    index.cast<int>());
            },
            py::arg("idx"))
        .def(
            "igetSegment",
            [](py::handle self, py::int_ seg_idx) { return self[seg_idx]; },
            py::arg("seg_idx"))
        .def("isMultiSegmentWell", &WellState::is_MSW)
        .def("hasSegmentData", &WellState::has_segment_data)
        .def(
            "__repr__",
            [](py::object py_self) {
                auto &self = py_self.cast<WellState &>();
                std::string msw = self.is_MSW() ? " (multi segment)" : "";
                std::string open = self.is_open() ? "open" : "shut";
                auto type =
                    py_self.attr("wellType")().attr("name").cast<std::string>();
                return fmt::format("WellState({}{}, "
                                   "number = {}, type = \"{}\", state = {})",
                                   self.get_name(), msw, self.get_well_nr(),
                                   type, open);
            })
        .def("gasRate", &WellState::get_gas_rate,
             "The gas rate, as stored in the restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "Mscf/day (field) or cm3/hour (lab). Use :meth:`gasRateSI` to get "
             "the\n"
             "value converted to SI units.\n")
        .def("waterRate", &WellState::get_water_rate,
             "The water rate, as stored in the restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "stb/day (field) or cm3/hour (lab). Use :meth:`waterRateSI` to "
             "get the\n"
             "value converted to SI units.\n")
        .def("oilRate", &WellState::get_oil_rate,
             "The oil rate, as stored in the restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "stb/day (field) or cm3/hour (lab). Use :meth:`oilRateSI` to get "
             "the\n"
             "value converted to SI units.\n")
        .def("volumeRate", &WellState::get_volume_rate,
             "The volume rate, at reservoir conditions, as stored in the "
             "restart file.\n"
             "\n"
             "The physical unit depends on the file's unit system: sm3/day "
             "(metric),\n"
             "stb/day (field) or cm3/hour (lab). Use :meth:`volumeRateSI` to "
             "get "
             "the\n"
             "value converted to SI units.\n")
        .def("gasRateSI", &WellState::get_gas_rate_si,
             "The gas rate converted to SI units (m3/s).\n"
             "\n"
             "This is the raw :meth:`gasRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n")
        .def("waterRateSI", &WellState::get_water_rate_si,
             "The water rate converted to SI units (m3/s).\n"
             "\n"
             "This is the raw :meth:`waterRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n")
        .def("oilRateSI", &WellState::get_oil_rate_si,
             "The oil rate converted to SI units (m3/s).\n"
             "\n"
             "This is the raw :meth:`oilRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n")
        .def("volumeRateSI", &WellState::get_volume_rate_si,
             "The volume rate, at reservoir conditions, converted to SI units "
             "(m3/s).\n"
             "\n"
             "This is the raw :meth:`volumeRate` multiplied by a unit-system "
             "dependent\n"
             "conversion factor.\n");
    m.attr("WellState") = well_state_module.attr("WellState");

    auto well_ts_module = m.def_submodule("well_time_line");
    py::class_<WellTimeLine, std::shared_ptr<WellTimeLine>>(well_ts_module,
                                                            "WellTimeLine")
        .def(py::init([]() -> std::shared_ptr<WellTimeLine> {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly");
            throw py::error_already_set();
        }))
        .def("getName", &WellTimeLine::name)
        .def("__len__", &WellTimeLine::size)
        .def(
            "__getitem__",
            [](py::object py_self, py::int_ index) {
                auto &self = py_self.cast<WellTimeLine &>();
                if (index < py::int_(0))
                    index += py::int_(self.size());
                if (!((py::int_(0) <= index) &&
                      (index < py::int_(self.size()))))
                    throw py::index_error(
                        fmt::format("Index must be in range 0 <= {} < {}",
                                    index.cast<long>(), self.size()));
                return self.at(index.cast<size_t>());
            },
            py::arg("index"))
        .def("__repr__", [](WellTimeLine &self) {
            return fmt::format("WellTimeLine(name = {}, size = {})",
                               self.name(), self.size());
        });
    m.attr("WellTimeLine") = well_ts_module.attr("WellTimeLine");

    auto well_info_module = m.def_submodule("well_info");
    py::class_<WellInfo>(well_info_module, "WellInfo")
        .def(py::init([](py::handle grid, std::optional<py::handle> rst_file,
                         bool load_segment_information) {
                 auto self = new WellInfo(from_cwrap<rd_grid_type>(grid));
                 if (rst_file.has_value()) {
                     if (py::isinstance<py::list>(*rst_file))
                         for (auto item : py::cast<py::list>(*rst_file))
                             load_rstfile(self, item, load_segment_information);
                     else
                         load_rstfile(self, *rst_file,
                                      load_segment_information);
                 }
                 return self;
             }),
             py::arg("grid"), py::arg("rst_file") = std::nullopt,
             py::arg("load_segment_information") = true)
        .def("__repr__",
             [](WellInfo &self) {
                 return fmt::format("WellInfo(well_count = {})",
                                    self.num_wells());
             })
        .def("__len__", &WellInfo::num_wells)
        .def(
            "__getitem__",
            [](WellInfo &self, std::variant<py::int_, std::string> item) {
                return std::visit(
                    overload{
                        [&self](std::string si) {
                            if (!self.has_well(si))
                                throw py::key_error(fmt::format(
                                    "The well '{}' is not in this set", si));
                            return self.get_ts(si);
                        },
                        [&self](py::int_ i) {
                            if ((i < py::int_(0)) ||
                                (i >= py::int_(self.num_wells())))
                                throw py::index_error(fmt::format(
                                    "Index must be in range 0 <= {} < {}",
                                    i.cast<long>(), self.num_wells()));
                            return self.get_ts(
                                self.get_well_name(i.cast<size_t>()));
                        },
                    },
                    item);
            },
            py::arg("item"))
        .def(
            "__iter__", [](WellInfo &self) { return WellIter(&self); },
            py::keep_alive<0, 1>())
        .def("allWellNames", &WellInfo::get_well_names)
        .def("__contains__", &WellInfo::has_well, py::arg("item"))
        .def("hasWell", &WellInfo::has_well, py::arg("well_name"))
        .def(
            "addWellFile",
            [](WellInfo &self, py::handle rst_file,
               bool load_segment_information) {
                load_rstfile(&self, rst_file, load_segment_information);
            },
            py::arg("rst_file"), py::arg("load_segment_information"));

    py::class_<WellIter>(well_info_module, "WellIter")
        .def("__iter__", [](WellIter &it) -> WellIter & { return it; })
        .def("__next__", &WellIter::next);

    m.attr("WellInfo") = well_info_module.attr("WellInfo");

    m.attr("__all__") =
        py::make_tuple("WellConnection", "WellConnectionDirection", "WellInfo",
                       "WellSegment", "WellState", "WellTimeLine", "WellType");
}
} // namespace
