#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_grid.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_grid, m) {
    m.doc() = "pybind11 bindings between rd_grid.py and rd_grid.cpp";

    m.def(
        "_fread_alloc",
        [](std::string filename, bool apply_mapaxes) {
            return reinterpret_cast<std::uintptr_t>(
                rd_grid_load_case__(filename.c_str(), apply_mapaxes));
        },
        py::return_value_policy::reference);
}
} // namespace
