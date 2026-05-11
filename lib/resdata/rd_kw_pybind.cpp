#include <cstdint>
#include <stdexcept>
#include <optional>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fmt/format.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_grdecl.hpp>
#include <resdata/rd_type.hpp>

namespace py = pybind11;

namespace {
template <typename T> T *from_cwrap(py::handle obj) {
    if (obj.is_none())
        return nullptr;

    py::int_ address = obj.attr("_BaseCClass__c_pointer");
    void *pointer = PyLong_AsVoidPtr(address.ptr());

    return reinterpret_cast<T *>(pointer);
}

PYBIND11_MODULE(_kw, m) {
    m.doc() = "pybind11 bindings between rd_kw.py and rd_kw.cpp";

    m.def(
        "_load_grdecl",
        [](py::handle file, std::optional<std::string> kw, bool strict,
           py::handle data_type) {
            auto *stream = from_cwrap<FILE>(file);
            auto *rd_data_type = from_cwrap<::rd_data_type>(data_type);
            if (rd_data_type == nullptr)
                throw std::invalid_argument("data_type must not be None");
            if (kw.has_value())
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_fscanf_alloc_grdecl_dynamic(stream, kw->c_str(),
                                                      strict, *rd_data_type));
            else
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_fscanf_alloc_grdecl_dynamic(stream, nullptr, strict,
                                                      *rd_data_type));
        },
        py::return_value_policy::reference);
    m.def("_fprintf_grdecl", [](py::handle self, py::handle file) {
        auto *stream = from_cwrap<FILE>(file);
        auto *kw = from_cwrap<rd_kw_type>(self);
        rd_kw_fprintf_grdecl(kw, stream);
    });
}
} // namespace
