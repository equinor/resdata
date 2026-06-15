#include <cstdint>
#include <stdexcept>
#include <optional>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fmt/format.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_grdecl.hpp>
#include <resdata/rd_type.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_kw, m) {
    m.doc() = "pybind11 bindings between rd_kw.py and rd_kw.cpp";

    m.def(
        "_load_grdecl",
        [](py::handle file, std::optional<std::string> kw, bool strict,
           py::handle data_type) {
            auto *stream = from_cwrap<FILE>(file);
            auto *rd_data_type = from_cwrap<::rd_data_type>(data_type);
            if (kw.has_value())
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_fscanf_alloc_grdecl(stream, kw->c_str(),
                                              *rd_data_type, 0, strict));
            else
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_fscanf_alloc_grdecl(stream, nullptr, *rd_data_type, 0,
                                              strict));
        },
        py::return_value_policy::reference);
    m.def("_fprintf_grdecl", [](py::handle self, py::handle file) {
        auto *stream = from_cwrap<FILE>(file);
        auto *kw = from_cwrap<rd_kw_type>(self);
        rd_kw_fprintf_grdecl(kw, stream);
    });
    m.def("_fseek_grdecl", [](std::string name, bool rewind, py::handle file) {
        auto *stream = from_cwrap<FILE>(file);
        return rd_kw_grdecl_fseek_kw(name.c_str(), rewind, stream);
    });
}
} // namespace
