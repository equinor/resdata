#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <memory>
#include <variant>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <pybind11/cast.h>
#include <pybind11/detail/common.h>
#include <pybind11/pytypes.h>
#include <pyerrors.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
py::object create_kw_reference(rd_kw_type *kw, py::object parent) {
    if (!kw)
        return py::none();
    return ResdataKW().attr("createCReference")(
        reinterpret_cast<std::uintptr_t>(kw), parent);
}
} // namespace

template <class... Ts> struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

PYBIND11_MODULE(rd_file_view, m) {
    register_exceptions(m);
    py::class_<rd::FileView, std::shared_ptr<rd::FileView>>(m,
                                                            "ResdataFileView")
        .def(py::init([]() -> std::shared_ptr<rd::FileView> {
            py::set_error(PyExc_NotImplementedError,
                          "Class can not be instantiated directly");
            throw py::error_already_set();
        }))
        .def("__repr__",
             [](rd::FileView &self) {
                 return fmt::format("ResdataFileView(size={})", self.size());
             })
        .def(
            "iget_named_kw",
            [](py::object py_self, std::string kw, py::int_ index) {
                auto &self = py_self.cast<rd::FileView &>();
                if (!self.has_kw(kw))
                    throw py::key_error(fmt::format("No such keyword: {}", kw));
                if (index < py::int_(0))
                    throw py::index_error(fmt::format(
                        "Index of iget_named_kw for {} was negative: {}", kw,
                        py::str(index).cast<std::string>()));
                if (index >= py::int_(self.num_named_kw(kw)))
                    throw py::index_error(
                        fmt::format("Too large index: {}",
                                    py::str(index).cast<std::string>()));
                return create_kw_reference(
                    self.get_kw(kw, index.cast<size_t>()), py_self);
            },
            py::arg("kw_name"), py::arg("index"))
        .def(
            "__getitem__",
            [](py::object py_self,
               std::variant<py::int_, py::slice, std::string> index)
                -> std::variant<py::object, std::vector<py::object>> {
                auto &self = py_self.cast<rd::FileView &>();
                return std::visit(
                    overload{
                        [&self, &py_self](std::string si) {
                            if (!self.has_kw(si))
                                throw py::key_error(fmt::format(
                                    "Unrecognized keyword: {}", si));
                            std::vector<py::object> result;
                            size_t n = self.num_named_kw(si);
                            result.reserve(n);
                            size_t i = 0;
                            std::generate_n(std::back_inserter(result), n,
                                            [&self, &py_self, &i, &si]() {
                                                return create_kw_reference(
                                                    self.get_kw(si, i++),
                                                    py_self);
                                            });
                            return std::variant<py::object,
                                                std::vector<py::object>>{
                                result};
                        },
                        [&self, &py_self](py::int_ start_index) {
                            py::int_ ls(self.size());
                            py::int_ index(start_index);
                            if (index < py::int_(0))
                                index += ls;

                            if (index < py::int_(0) || index >= ls)
                                throw py::index_error(fmt::format(
                                    "Index must be in [0, {}), was: {}.",
                                    py::str(ls).cast<std::string>(),
                                    py::str(index).cast<std::string>()));

                            return std::variant<py::object,
                                                std::vector<py::object>>{
                                create_kw_reference(
                                    self.get_kw(index.cast<size_t>()),
                                    py_self)};
                        },
                        [&self, &py_self](py::slice s) {
                            size_t size = self.size();
                            if (size > (size_t)PY_SSIZE_T_MAX)
                                throw std::invalid_argument(
                                    "Size overflowed in slice calculation");
                            auto ssize = static_cast<py::ssize_t>(self.size());

                            py::ssize_t start, stop, step, slicelength;
                            if (!s.compute(ssize, &start, &stop, &step,
                                           &slicelength)) {
                                throw py::index_error("slice out of range");
                            }

                            std::vector<py::object> result;
                            result.reserve(slicelength);
                            for (py::ssize_t i = 0; i < slicelength; ++i) {
                                py::ssize_t idx = start + i * step;
                                if (idx < 0)
                                    throw py::index_error(
                                        fmt::format("Negative index {} in "
                                                    "FileView::__getitem__",
                                                    idx));
                                result.push_back(create_kw_reference(
                                    self.get_kw(static_cast<size_t>(idx)),
                                    py_self));
                            }
                            return std::variant<py::object,
                                                std::vector<py::object>>{
                                result};
                        },
                    },
                    index);
            },
            py::arg("index"),
            "Look up ResdataKW instances from the current ResdataFile.\n"
            "\n"
            "The index argument can either be an integer, in\n"
            "which case the method will return ResdataKW number index, or\n"
            "alternatively a keyword string, in which case the method will\n"
            "return a list of ResdataKW instances with that keyword:\n"
            "\n"
            "   restart_file = rd_file.ResdataFile(\"CASE.UNRST\")\n"
            "   kw9 = restart_file[9]\n"
            "   swat_list = restart_file[\"SWAT\"]\n"
            "\n"
            "The keyword based lookup can be combined with an extra [] to\n"
            "get ResdataKW instance nr:\n"
            "\n"
            "   swat9 = restart_file[\"SWAT\"][9]\n"
            "\n"
            "Will return the 10'th SWAT keyword from the restart file. The\n"
            "following example will iterate over all the SWAT keywords in a\n"
            "restart file:\n"
            "\n"
            "   restart_file = rd_file.ResdataFile(\"CASE.UNRST\")\n"
            "   for swat in restart_file[\"SWAT\"]:\n"
            "       ....\n")
        .def("__len__", &rd::FileView::size)
        .def("__contains__", &rd::FileView::has_kw)
        .def("num_keywords", &rd::FileView::num_named_kw, py::arg("kw"))
        .def("unique_size", &rd::FileView::num_distinct_kw)
        .def("unique_kw", &rd::FileView::get_distinct_kw)
        .def(
            "block_view2",
            [](rd::FileView *self, std::optional<std::string> start_kw,
               std::optional<std::string> stop_kw, py::int_ start_index) {
                size_t occurence = 0;
                if (start_kw) {
                    size_t num_start = self->num_named_kw(*start_kw);
                    if (num_start == 0)
                        throw py::key_error(fmt::format(
                            "The keyword:{} is not in file", *start_kw));
                    py::int_ index(start_index);
                    py::int_ ls(num_start);
                    if (index < py::int_(0))
                        index += ls;

                    if (index < py::int_(0) || index >= ls)
                        throw py::index_error(fmt::format(
                            "Index must be in [0, {}), was: {}.",
                            py::str(ls).cast<std::string>(),
                            py::str(start_index).cast<std::string>()));
                    occurence = index.cast<size_t>();
                }
                if (stop_kw && !self->has_kw(*stop_kw))
                    throw py::key_error(
                        fmt::format("The keyword:{} is not in file", *stop_kw));

                return self->blockview(start_kw, stop_kw, occurence);
            },
            py::arg("start_kw"), py::arg("stop_kw"), py::arg("start_index"))
        .def(
            "block_view",
            [](rd::FileView *self, std::string kw, py::int_ start_index) {
                size_t num = self->num_named_kw(kw);

                if (num == 0)
                    throw py::key_error(fmt::format("Unknown keyword: {}", kw));

                py::int_ ls(num);
                py::int_ index(start_index);
                if (index < py::int_(0))
                    index += ls;

                if (index < py::int_(0) || index >= ls)
                    throw py::index_error(
                        fmt::format("Index must be in [0, {}), was: {}.", num,
                                    py::str(index).cast<std::string>()));

                return self->blockview(kw, kw, index.cast<size_t>());
            },
            py::arg("kw"), py::arg("kw_index"))
        .def(
            "restart_view",
            [](rd::FileView &self, std::optional<py::int_> seqnum_index,
               std::optional<int> report_step, py::object sim_time,
               std::optional<double> sim_days) {
                std::shared_ptr<rd::FileView> view(nullptr);
                if (seqnum_index.has_value()) {
                    if (*seqnum_index < py::int_(0))
                        throw std::invalid_argument(
                            "seqnum_index cannot be negative");
                    view = self.restart_view_from_seqnum_index(
                        (*seqnum_index).cast<size_t>());
                } else if (report_step.has_value())
                    view = self.restart_view_from_report_step(*report_step);
                else if (!sim_time.is_none())
                    view = self.restart_view_from_sim_time(
                        CTime()(sim_time).attr("value")().cast<time_t>());
                else if (sim_days.has_value())
                    view = self.restart_view_from_sim_days(*sim_days);

                if (!view)
                    throw py::value_error(
                        "No such restart block could be identified");
                return view;
            },
            py::arg("seqnum_index") = py::none(),
            py::arg("report_step") = py::none(),
            py::arg("sim_time") = py::none(), py::arg("sim_days") = py::none());
}
