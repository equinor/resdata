#include <cstdint>
#include <cstdio>
#include <ctime>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/pytypes.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file_flag.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
py::object create_kw_reference(rd_kw_type *kw, py::object parent) {
    if (!kw)
        return py::none();
    return ResdataKW().attr("createCReference")(
        reinterpret_cast<std::uintptr_t>(kw), parent);
}

std::vector<py::object> named_kw_list(py::object py_self, rd::FileView &view,
                                      const std::string &kw) {
    if (!view.has_kw(kw))
        throw py::key_error(fmt::format("Unrecognized keyword: {}", kw));
    std::vector<py::object> result;
    size_t n = view.num_named_kw(kw);
    result.reserve(n);
    for (size_t i = 0; i < n; ++i)
        result.push_back(create_kw_reference(view.get_kw(kw, i), py_self));
    return result;
}

py::list report_list_impl(py::object parent, rd::FileView &view,
                          const std::string &fname) {
    py::list report_steps;
    if (view.has_kw("SEQNUM")) {
        for (auto &kw : named_kw_list(parent, view, "SEQNUM"))
            report_steps.append(kw[py::int_(0)]);
    } else {
        std::smatch match;
        std::regex re(R"(\.[XF](\d{4})$)");
        std::string f = fname;
        if (std::regex_search(f, match, re)) {
            report_steps.append(py::int_(std::stoi(match[1].str())));
        } else {
            throw py::type_error(
                fmt::format("Tried get list of report steps from file \"{}\" - "
                            "which is not a restart file",
                            fname));
        }
    }
    return report_steps;
}
} // namespace

template <class... Ts> struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overload(Ts...) -> overload<Ts...>;

PYBIND11_MODULE(rd_file, m) {
    register_exceptions(m);
    m.doc() =
        "The rd_file module contains functionality to load a file\n"
        "in 'restart format'. Files of 'restart format' include restart "
        "files,\n"
        "init files, grid files, summary files and RFT files.\n"
        "\n"
        "The rd_file implementation is agnostic[1] to the content and\n"
        "structure of the file; more specialized classes like Summary and\n"
        "Grid use the ResdataFile functionality for low level file loading.\n"
        "\n"
        "The typical usage involves loading a complete file, and then\n"
        "subsequently querying for various keywords. In the example below we\n"
        "load a restart file, and ask for the SWAT keyword:\n"
        "\n"
        "   file = ResdataFile( \"CASE.X0067\" )\n"
        "   swat_kw = file.iget_named_kw( \"SWAT\" , 0 )\n"
        "\n"
        "[1]: In particular for restart files, which do not have a special\n"
        "     RestartFile class, there is some specialized functionality.\n";

    py::class_<rd::File>(m, "ResdataFile", py::dynamic_attr())
        .def(py::init([](std::string filename, FileMode flags,
                         std::optional<std::string> index_filename)
                          -> std::unique_ptr<rd::File> {
                 if (!index_filename)
                     return rd::File::open(filename, flags);
                 return rd::File::fast_open(filename, *index_filename, flags);
             }),
             py::arg("filename"),
             py::arg_v("flags", FileMode::DEFAULT, "FileMode.DEFAULT"),
             py::arg("index_filename") = py::none(),
             "Loads the complete file filename.\n"
             "\n"
             "Will create a new ResdataFile instance with the content of file\n"
             "@filename. The file @filename must be in 'restart format' -\n"
             "otherwise it will be crash and burn.\n"
             "\n"
             "The optional argument flags can be an or'ed combination of the\n"
             "flags:\n"
             "\n"
             "   WRITABLE : It is possible to update the\n"
             "      content of the keywords in the file.\n"
             "\n"
             "   CLOSE_STREAM : The underlying FILE * is closed\n"
             "      when not used; to save number of open file descriptors\n"
             "      in cases where a high number of ResdataFile instances are\n"
             "      open concurrently.\n"
             "\n"
             "When the file has been loaded the ResdataFile instance can be\n"
             "used to query for and get reference to the ResdataKW instances\n"
             "constituting the file, like e.g. SWAT from a restart file or\n"
             "FIPNUM from an INIT file.\n")
        .def_static(
            "get_filetype",
            [](std::string filename) -> py::tuple {
                bool fmt_file = false;
                int report_step = -1;
                auto file_type =
                    rd_get_file_type(filename.c_str(), &fmt_file, &report_step);
                py::object report =
                    report_step == -1 ? py::none() : py::cast(report_step);
                if (file_type != FileType::RESTART &&
                    file_type != FileType::SUMMARY)
                    report = py::none();

                py::object fmt = (file_type == FileType::OTHER ||
                                  file_type == FileType::DATA)
                                     ? py::object(py::none())
                                     : py::cast(fmt_file);

                return py::make_tuple(file_type, report, fmt);
            },
            py::arg("filename"))
        .def_static(
            "contains_report_step",
            [](std::string filename, int report_step) {
                auto obj = rd::File::open(filename);
                return obj->has_report_step(report_step);
            },
            py::arg("filename"), py::arg("report_step"),
            "Will check if the filename contains report_step.\n"
            "\n"
            "This staticmethod works by opening the file @filename and\n"
            "searching through linearly to see if an rd_kw with value\n"
            "corresponding to @report_step can be found.\n"
            "\n"
            "If you have already loaded the file into an ResdataFile instance\n"
            "you should use the has_report_step() method instead.\n")
        .def_static(
            "contains_sim_time",
            [](std::string filename, py::object dtime) {
                auto obj = rd::File::open(filename);
                return obj->has_sim_time(
                    CTime()(dtime).attr("value")().cast<time_t>());
            },
            py::arg("filename"), py::arg("dtime"),
            "Will check if the @filename contains simulation at @dtime.\n"
            "\n"
            "This staticmethod works by opening the file filename and\n"
            "searching through linearly to see if a result block at the\n"
            "time corresponding to @dtime can be found.\n"
            "\n"
            "If you have already loaded the file into an ResdataFile instance\n"
            "you should use the has_sim_time() method instead.\n")
        .def_static(
            "file_report_list",
            [](std::string filename) {
                auto obj = rd::File::open(filename);
                return report_list_impl(py::none(), *obj->get_global_view(),
                                        obj->filename());
            },
            py::arg("filename"),
            "Will identify the available report_steps from @filename.\n")
        .def_property_readonly("report_list",
                               [](py::object py_self) -> py::list {
                                   auto &self = py_self.cast<rd::File &>();
                                   return report_list_impl(
                                       py_self, *self.get_global_view(),
                                       self.filename());
                               })
        .def("__repr__",
             [](rd::File &self) {
                 std::string fn = self.filename();
                 std::string wr = self.is_writable() ? ", read/write" : "";
                 return fmt::format("ResdataFile(\"{}\"{})", fn, wr);
             })
        .def(
            "save_kw",
            [](rd::File &self, py::handle kw) {
                if (self.is_writable()) {
                    self.save_kw(from_cwrap<rd_kw_type>(kw));
                } else {
                    PyErr_SetString(
                        PyExc_OSError,
                        fmt::format("save_kw: the file \"{}\" has been opened "
                                    "read only.",
                                    self.filename())
                            .c_str());
                    throw py::error_already_set();
                }
            },
            py::arg("kw"),
            "Will write the @kw back to file.\n"
            "\n"
            "This function should typically be used in situations like this:\n"
            "\n"
            "  1. Create an ResdataFile instance around a restart format "
            "file.\n"
            "  2. Extract a keyword of interest and modify it.\n"
            "  3. Call this method to save the modifications to disk.\n"
            "\n"
            "There are several restrictions to the use of this function:\n"
            "\n"
            "  1. The ResdataFile instance must have been created with the\n"
            "     optional read_only flag set to False.\n"
            "\n"
            "  2. You can only modify the content of the keyword; if you\n"
            "     try to modify the header in any way (i.e. size, datatype\n"
            "     or name) the function will fail.\n"
            "\n"
            "  3. The keyword you are trying to save must be exactly the\n"
            "     keyword you got from this ResdataFile instance, otherwise\n"
            "     the function will fail.\n")
        .def("__len__", &rd::File::size)
        .def(
            "close", &rd::File::close,
            "Closes the file handle used to read data.\n"
            "\n"
            "There are two caveats:\n"
            "\n"
            "  1. Stale cached ResdataKW instances: A ResdataKW that was\n"
            "     already read before close() keeps the data it held at that\n"
            "     time. It is a snapshot: it is not refreshed and will not\n"
            "     reflect any later modifications made to the file.\n"
            "\n"
            "  2. Lazy re-opening of ResdataFileView: Keyword data is loaded\n"
            "     lazily, so a keyword that was not yet read when close() was\n"
            "     called is read on first access. Accessing such a keyword\n"
            "     through a ResdataFileView (or through this ResdataFile)\n"
            "     re-opens the file on disk to read it.\n")
        .def(
            "block_view",
            [](py::object py_self, std::string kw, py::int_ kw_index) {
                auto &self = py_self.cast<rd::File &>();
                auto view = self.get_global_view();
                if (!view->has_kw(kw))
                    throw py::key_error(
                        fmt::format("No such keyword \"{}\".", kw));
                py::int_ ls(view->num_named_kw(kw));
                py::int_ idx(kw_index);
                if (idx < py::int_(0))
                    idx += ls;
                if (idx >= py::int_(0) && idx < ls)
                    return view->blockview(kw, kw, idx.cast<size_t>());
                throw py::index_error(fmt::format(
                    "Index out of range, must be in [0, {}), was {}.",
                    py::str(ls).cast<std::string>(),
                    py::str(kw_index).cast<std::string>()));
            },
            py::arg("kw"), py::arg("kw_index"),
            "A view of the keyword block delimited by kw.\n"
            "\n"
            "The returned ResdataFileView contains the keywords from the\n"
            "kw_index'th occurrence of kw (inclusive) up to, but not\n"
            "including, the next occurrence of kw. In other words the file is\n"
            "treated as a sequence of blocks that each start with kw, and\n"
            "this method returns one such block:\n"
            "\n"
            "    # File: HEADER DATA1 DATA2 HEADER DATA1 DATA2\n"
            "    view = rd_file.block_view(\"HEADER\", 1)\n"
            "    # view contains: HEADER DATA1 DATA2   (the second block)\n"
            "\n"
            "The last block extends to the end of the file.\n"
            "\n"
            "kw_index selects which occurrence of kw starts the block. A\n"
            "negative index counts from the end, so -1 is the last\n"
            "occurrence.\n"
            "\n"
            ":param kw: The keyword that delimits the blocks.\n"
            ":param kw_index: Which occurrence of kw to start the block at.\n"
            ":raises KeyError: If kw is not present in the file.\n"
            ":raises IndexError: If kw_index is out of range.\n")
        .def(
            "block_view2",
            [](rd::File &self, std::optional<std::string> start_kw,
               std::optional<std::string> stop_kw, py::int_ start_index) {
                auto view = self.get_global_view();
                size_t occurence = 0;
                if (start_kw) {
                    size_t num_start = view->num_named_kw(*start_kw);
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
                if (stop_kw && !view->has_kw(*stop_kw))
                    throw py::key_error(
                        fmt::format("The keyword:{} is not in file", *stop_kw));
                return view->blockview(start_kw, stop_kw, occurence);
            },
            py::arg("start_kw"), py::arg("stop_kw"), py::arg("start_index"),
            "Return a view of the keywords between start_kw and stop_kw.\n"
            "\n"
            "The returned ResdataFileView starts at the start_index'th\n"
            "occurrence of start_kw (inclusive) and ends just before the\n"
            "first occurrence of stop_kw that follows it (exclusive):\n"
            "\n"
            "    # File: SEQNUM PRESSURE SWAT PRESSURE SWAT\n"
            "    view = rd_file.block_view2(\"SEQNUM\", \"PRESSURE\", 0)\n"
            "    # view contains: SEQNUM   (up to, but not including, "
            "PRESSURE)\n"
            "\n"
            "start_kw and stop_kw may be None:\n"
            "\n"
            "* If start_kw is None the view starts at the first keyword in\n"
            "  the file and start_index is ignored.\n"
            "* If stop_kw is None the view extends to the end of the file.\n"
            "\n"
            "So block_view2(None, None, 0) returns a view containing every\n"
            "keyword in the file, in order:\n"
            "\n"
            "    # File: SEQNUM PRESSURE SWAT PRESSURE SWAT\n"
            "    view = rd_file.block_view2(None, None, 0)\n"
            "    # view contains: SEQNUM PRESSURE SWAT PRESSURE SWAT\n"
            "\n"
            "start_index selects which occurrence of start_kw to start at. A\n"
            "negative index counts from the end, so -1 is the last\n"
            "occurrence.\n"
            "\n"
            ":param start_kw: The keyword to start at, or None for the start\n"
            "    of the file.\n"
            ":param stop_kw: The keyword to stop before, or None for the end\n"
            "    of the file.\n"
            ":param start_index: Which occurrence of start_kw to start at.\n"
            ":raises KeyError: If start_kw or stop_kw is given but not\n"
            "    present in the file.\n"
            ":raises IndexError: If start_index is out of range.\n")
        .def(
            "restart_view",
            [](rd::File &self, std::optional<py::int_> seqnum_index,
               std::optional<int> report_step, py::object sim_time,
               std::optional<double> sim_days) {
                auto view = self.get_global_view();
                std::shared_ptr<rd::FileView> result(nullptr);
                if (seqnum_index.has_value()) {
                    if (*seqnum_index < py::int_(0))
                        throw std::invalid_argument(
                            "seqnum_index cannot be negative");
                    result = view->restart_view_from_seqnum_index(
                        (*seqnum_index).cast<size_t>());
                } else if (report_step.has_value())
                    result = view->restart_view_from_report_step(*report_step);
                else if (!sim_time.is_none())
                    result = view->restart_view_from_sim_time(
                        CTime()(sim_time).attr("value")().cast<time_t>());
                else if (sim_days.has_value())
                    result = view->restart_view_from_sim_days(*sim_days);

                if (!result)
                    throw py::value_error(
                        "No such restart block could be identified");
                return result;
            },
            py::arg("seqnum_index") = py::none(),
            py::arg("report_step") = py::none(),
            py::arg("sim_time") = py::none(), py::arg("sim_days") = py::none())
        .def(
            "__getitem__",
            [](py::object py_self, std::variant<py::int_, std::string> index)
                -> std::variant<py::object, std::vector<py::object>> {
                auto &self = py_self.cast<rd::File &>();
                auto global = self.get_global_view();
                auto &view = *global;
                return std::visit(
                    overload{
                        [&](std::string si)
                            -> std::variant<py::object,
                                            std::vector<py::object>> {
                            return named_kw_list(py_self, view, si);
                        },
                        [&](py::int_ index)
                            -> std::variant<py::object,
                                            std::vector<py::object>> {
                            py::int_ ls(view.size());
                            py::int_ idx(index);
                            if (idx < py::int_(0))
                                idx += ls;
                            if (idx < py::int_(0) || idx >= ls)
                                throw py::index_error(fmt::format(
                                    "Index must be in [0, {}), was: {}.",
                                    py::str(ls).cast<std::string>(),
                                    py::str(index).cast<std::string>()));
                            return create_kw_reference(
                                view.get_kw(idx.cast<size_t>()), py_self);
                        },
                    },
                    index);
            },
            py::arg("index"),
            "Implements [] operator; index can be integer or key.\n"
            "\n"
            "Will look up ResdataKW instances from the current ResdataFile\n"
            "instance. The index argument can either be an integer, in\n"
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
        .def(
            "iget_kw",
            [](py::object py_self, py::int_ index, bool copy) {
                py::object kw = py_self.attr("__getitem__")(index);
                if (copy)
                    return ResdataKW().attr("copy")(kw);
                return kw;
            },
            py::arg("index"), py::arg("copy") = false,
            "Will return ResdataKW instance nr @index.\n"
            "\n"
            "In the files loaded with the ResdataFile implementation the\n"
            "keywords come sequentially in a long series, an INIT\n"
            "file might have the following keywords:\n"
            "\n"
            "  INTEHEAD\n"
            "  LOGIHEAD\n"
            "  DOUBHEAD\n"
            "  PORV\n"
            "  DX\n"
            "  DY\n"
            "  DZ\n"
            "  PERMX\n"
            "  PERMY\n"
            "  PERMZ\n"
            "  MULTX\n"
            "  MULTY\n"
            "  .....\n"
            "\n"
            "The iget_kw() method will give you a ResdataKW reference to\n"
            "keyword nr @index. This functionality is also available\n"
            "through the index operator []:\n"
            "\n"
            "   file = ResdataFile( \"CASE.INIT\" )\n"
            "   permx = file.iget_kw( 7 )\n"
            "   permz = file[ 9 ]\n"
            "\n"
            "Observe that the returned ResdataKW instance is only a reference\n"
            "to the data owned by the ResdataFile instance.\n"
            "\n"
            "The method iget_named_kw() which lets you specify the name of\n"
            "the keyword you are interested in is in general more useful\n"
            "than this method.\n")
        .def(
            "iget_named_kw",
            [](py::object py_self, std::string kw_name, py::int_ index,
               bool copy) {
                auto &self = py_self.cast<rd::File &>();
                auto view = self.get_global_view();
                if (!view->has_kw(kw_name))
                    throw py::key_error(
                        fmt::format("No such keyword: {}", kw_name));
                if (index < py::int_(0))
                    throw py::index_error(fmt::format(
                        "Index of iget_named_kw for {} was negative: {}",
                        kw_name, py::str(index).cast<std::string>()));
                if (index >= py::int_(view->num_named_kw(kw_name)))
                    throw py::index_error(
                        fmt::format("Too large index: {}",
                                    py::str(index).cast<std::string>()));
                return create_kw_reference(
                    view->get_kw(kw_name, index.cast<size_t>()), py_self);
            },
            py::arg("kw_name"), py::arg("index"), py::arg("copy") = false)
        .def(
            "restart_get_kw",
            [](py::object py_self, std::string kw_name, py::object dtime,
               bool copy) -> py::object {
                auto &self = py_self.cast<rd::File &>();
                auto index = self.find_sim_time(
                    CTime()(dtime).attr("value")().cast<time_t>());
                if (index.has_value()) {
                    if (self.num_named_kw(kw_name) > index.value()) {
                        py::object kw = py_self.attr("iget_named_kw")(
                            kw_name, py::int_(index.value()));
                        if (copy)
                            return ResdataKW().attr("copy")(kw);
                        return kw;
                    }
                    if (self.has_kw(kw_name))
                        throw py::index_error(fmt::format(
                            "Does not have keyword \"{}\" at time:{}.", kw_name,
                            py::str(dtime).cast<std::string>()));
                    throw py::key_error(
                        fmt::format("Keyword \"{}\" not recognized.", kw_name));
                }
                throw py::index_error(
                    fmt::format("Does not have keyword \"{}\" at time:{}.",
                                kw_name, py::str(dtime).cast<std::string>()));
            },
            py::arg("kw_name"), py::arg("dtime"), py::arg("copy") = false,
            "Will return ResdataKW @kw_name from restart file at time dtime.\n"
            "\n"
            "This function assumes that the current ResdataFile instance\n"
            "represents a restart file. It will then look for keyword\n"
            "@kw_name exactly at the time @dtime; @dtime is a datetime\n"
            "instance:\n"
            "\n"
            "    file = ResdataFile( \"CASE.UNRST\" )\n"
            "    swat2010 = file.restart_get_kw( \"SWAT\" , "
            "datetime.datetime( 2000 , 1 , 1 ))\n"
            "\n"
            "By default the returned kw instance is a reference to the\n"
            "rd_kw still contained in the ResdataFile instance; i.e. the kw\n"
            "will become a dangling reference if the ResdataFile instance\n"
            "goes out of scope. If the optional argument copy is True the\n"
            "returned kw will be a true copy.\n"
            "\n"
            "If the file does not have the keyword at the specified time\n"
            "the function will raise IndexError(); if the file does not\n"
            "have the keyword at all - KeyError will be raised.\n")
        .def_property_readonly(
            "size", &rd::File::size,
            "The number of keywords in the current ResdataFile object.\n")
        .def_property_readonly(
            "unique_size",
            [](rd::File &self) {
                return self.get_global_view()->num_distinct_kw();
            },
            "The number of unique keyword (names) in the current ResdataFile "
            "object.\n")
        .def(
            "keys",
            [](py::object py_self) {
                auto &self = py_self.cast<rd::File &>();
                auto view = self.get_global_view();
                py::dict header_dict;
                size_t n = view->size();
                for (size_t index = 0; index < n; ++index) {
                    py::object kw =
                        create_kw_reference(view->get_kw(index), py_self);
                    header_dict[kw.attr("get_name")()] = py::bool_(true);
                }
                return header_dict.attr("keys")();
            },
            "Will return a list of unique kw names - like keys() on a dict.\n")
        .def_property_readonly(
            "headers",
            [](py::object py_self) {
                auto &self = py_self.cast<rd::File &>();
                auto view = self.get_global_view();
                py::list header_list;
                size_t n = view->size();
                for (size_t index = 0; index < n; ++index) {
                    py::object kw =
                        create_kw_reference(view->get_kw(index), py_self);
                    header_list.append(kw.attr("header"));
                }
                return header_list;
            },
            "Will return a list of the headers of all the keywords.\n")
        .def_property_readonly(
            "report_steps",
            [](rd::File &self) {
                auto view = self.get_global_view();
                size_t num_steps = self.num_named_kw("SEQNUM");
                py::list steps;
                for (size_t i = 0; i < num_steps; i++)
                    steps.append(rd_kw_iget_int(self.get_kw("SEQNUM", i), 0));
                return steps;
            },
            "Will return a list of all report steps.\n"
            "\n"
            "The method works by iterating through the whole restart file\n"
            "looking for 'SEQNUM' keywords; if the current ResdataFile\n"
            "instance is not a restart file it will not contain any 'SEQNUM'\n"
            "keywords and the method will simply return an empty list.\n")
        .def_property_readonly(
            "report_dates",
            [](py::object py_self) -> py::object {
                auto &self = py_self.cast<rd::File &>();
                auto view = self.get_global_view();
                if (view->has_kw("SEQNUM")) {
                    py::list dates;
                    size_t n = view->num_named_kw("SEQNUM");
                    for (size_t index = 0; index < n; ++index)
                        dates.append(py_self.attr("iget_restart_sim_time")(
                            py::int_(index)));
                    return dates;
                }
                if (view->has_kw("INTEHEAD")) {
                    auto intehead = view->get_kw("INTEHEAD", 0);
                    int year = rd_kw_iget_int(intehead, 66);
                    int month = rd_kw_iget_int(intehead, 65);
                    int day = rd_kw_iget_int(intehead, 64);
                    py::object datetime =
                        py::module_::import("datetime").attr("datetime");
                    py::list dates;
                    dates.append(datetime(py::int_(year), py::int_(month),
                                          py::int_(day)));
                    return dates;
                }
                return py::none();
            },
            "Will return a list of the dates for all report steps.\n"
            "\n"
            "The method works by iterating through the whole restart file\n"
            "looking for 'SEQNUM/INTEHEAD' keywords; the method can\n"
            "probably be tricked by other file types also containing an\n"
            "INTEHEAD keyword.\n"
            "\n"
            "If the file contains neither 'SEQNUM' nor 'INTEHEAD' keywords\n"
            "None is returned.\n")
        .def_property_readonly(
            "dates",
            [](py::object py_self) { return py_self.attr("report_dates"); },
            "Will return a list of the dates for all report steps.\n"
            "\n"
            "If the file contains neither 'SEQNUM' nor 'INTEHEAD' keywords\n"
            "None is returned.\n")
        .def("num_named_kw", &rd::File::num_named_kw, py::arg("kw"),
             "The number of keywords with name == @kw in the current "
             "ResdataFile object.\n")
        .def(
            "has_kw",
            [](rd::File &self, std::string kw, py::int_ num) {
                if (num < py::int_(0))
                    return true;
                if (num > py::int_(std::numeric_limits<size_t>::max()))
                    return false;
                return self.num_named_kw(kw) > num.cast<size_t>();
            },
            py::arg("kw"), py::arg("num") = 0,
            "Check if current ResdataFile instance has a keyword @kw.\n"
            "\n"
            "If the optional argument num is given it will check if the\n"
            "ResdataFile has more than num occurences of @kw.\n")
        .def("__contains__", &rd::File::has_kw, py::arg("kw"),
             "Check if the current file contains keyword @kw.\n")
        .def("has_report_step", &rd::File::has_report_step,
             py::arg("report_step"),
             "Checks if the current ResdataFile has report step @report_step.\n"
             "\n"
             "If the ResdataFile in question is not a restart file, you will\n"
             "just get False. If you want to check if the file contains the\n"
             "actual report_step before loading the file, you should use the\n"
             "classmethod contains_report_step() instead.\n")
        .def(
            "num_report_steps",
            [](rd::File &self) { return self.num_named_kw("SEQNUM"); },
            "Returns the total number of report steps in the restart file.\n"
            "\n"
            "Works by counting the number of 'SEQNUM' instances, and will\n"
            "happily return 0 for a non-restart file. Observe that the\n"
            "report_steps present in a unified restart file are in general\n"
            "not consecutive, i.e. the last report step will typically be\n"
            "much higher than the return value from this function.\n")
        .def(
            "has_sim_time",
            [](rd::File &self, py::object dtime) {
                return self.has_sim_time(
                    CTime()(dtime).attr("value")().cast<time_t>());
            },
            py::arg("dtime"),
            "Checks if the current ResdataFile has data for time @dtime.\n"
            "\n"
            "The implementation goes through all the INTEHEAD headers in the\n"
            "ResdataFile, i.e. it can fail if the ResdataFile instance in\n"
            "question has INTEHEAD keyword(s), but is still not a restart\n"
            "file. The @dtime argument should be a normal python datetime\n"
            "instance.\n")
        .def(
            "iget_restart_sim_time",
            [](rd::File &self, size_t index) {
                return CTime()(py::int_(static_cast<std::int64_t>(
                                   self.restart_sim_date(index))))
                    .attr("datetime")();
            },
            py::arg("index"),
            "Will locate restart block nr @index and return the true time\n"
            "as a datetime instance.\n")
        .def("iget_restart_sim_days", &rd::File::restart_sim_days,
             py::arg("index"),
             "Will locate restart block nr @index and return the number of\n"
             "days (in METRIC at least ...) since the simulation started.\n")
        .def("get_filename", &rd::File::filename,
             "Name of the file currently loaded.\n")
        .def(
            "fwrite",
            [](rd::File &self, ERT::FortIO &fortio) { self.write(fortio, 0); },
            py::arg("fortio"),
            "Will write current ResdataFile instance to fortio stream.\n"
            "\n"
            "ECLIPSE is written in Fortran; and a \"special\" handle for\n"
            "Fortran IO must be used when reading and writing these files.\n"
            "This method will write the current ResdataFile instance to a\n"
            "FortIO stream already opened for writing:\n"
            "\n"
            "   >>> from resdata.resfile import FortIO\n"
            "\n"
            "   >>> fortio = FortIO( \"FILE.XX\" )\n"
            "   >>> file.fwrite( fortio )\n"
            "   >>> fortio.close()\n")
        .def(
            "write_index",
            [](rd::File &self, const std::string &index_file_name) {
                if (self.size() == 0)
                    throw std::ios_base::failure(
                        fmt::format("Cannot write index for empty file:{}",
                                    index_file_name));
                self.write_index(index_file_name);
            },
            py::arg("index_file_name"))
        .def_property_readonly("global_view", &rd::File::get_global_view);
}
