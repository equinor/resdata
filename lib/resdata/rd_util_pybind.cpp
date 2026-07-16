#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_util.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_rd_util, m) {
    m.doc() = "pybind11 bindings for the FileMode and FileType enums from "
              "rd_file_flag.hpp and rd_util.hpp";

    py::enum_<FileMode> file_mode(m, "FileMode", py::arithmetic());
    file_mode.value("DEFAULT", FileMode::DEFAULT)
        .value("CLOSE_STREAM", FileMode::CLOSE_STREAM)
        .value("WRITABLE", FileMode::WRITABLE);

    file_mode.def(
        "__or__", [](FileMode a, FileMode b) { return a | b; },
        py::is_operator());
    file_mode.def(
        "__and__", [](FileMode a, FileMode b) { return a & b; },
        py::is_operator());
    file_mode.def(
        "__xor__", [](FileMode a, FileMode b) { return a ^ b; },
        py::is_operator());
    file_mode.def(
        "__invert__", [](FileMode a) { return ~a; }, py::is_operator());

    py::enum_<FileType>(m, "FileType")
        .value("OTHER", FileType::OTHER)
        .value("RESTART", FileType::RESTART)
        .value("UNIFIED_RESTART", FileType::UNIFIED_RESTART)
        .value("SUMMARY", FileType::SUMMARY)
        .value("UNIFIED_SUMMARY", FileType::UNIFIED_SUMMARY)
        .value("SUMMARY_HEADER", FileType::SUMMARY_HEADER)
        .value("GRID", FileType::GRID)
        .value("EGRID", FileType::EGRID)
        .value("INIT", FileType::INIT)
        .value("RFT", FileType::RFT)
        .value("DATA", FileType::DATA);
    m.def("_get_file_type", [](std::string filename) {
        bool fmt_file = false;
        int report_step = -1;
        auto file_type =
            rd_get_file_type(filename.c_str(), &fmt_file, &report_step);
        return std::make_tuple(file_type, fmt_file, report_step);
    });
}
} // namespace
