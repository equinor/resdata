#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <resdata/rd_file_flag.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_file_mode, m) {
    m.doc() = "pybind11 bindings for the FileMode enum from rd_file_flag.hpp";

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
}
} // namespace
