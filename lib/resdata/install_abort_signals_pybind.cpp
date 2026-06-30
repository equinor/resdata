#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/util.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_install_abort_signals, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for install_abort_signals";

    m.def("_install_signals", []() { util_install_signals(); });
    m.def("_update_signals", []() { util_update_signals(); });
}
