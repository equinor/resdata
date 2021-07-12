#include <pybind11/pybind11.h>
namespace py = pybind11;

void init_native_summary(py::module_);

PYBIND11_MODULE(_native, m) { init_native_summary(m.def_submodule("summary")); }
