#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/rng.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
PYBIND11_MODULE(_rng, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for RandomNumberGenerator";

    m.def(
        "_rng_alloc",
        [](int alg_type, int init_mode) {
            return reinterpret_cast<std::uintptr_t>(rng_alloc(
                static_cast<rng_alg_type>(alg_type),
                static_cast<rng_init_mode>(init_mode)));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) { rng_free(from_cwrap<rng_type>(self)); });
    m.def("_get_double", [](py::handle self) {
        return rng_get_double(from_cwrap<rng_type>(self));
    });
    m.def("_get_int", [](py::handle self, std::int64_t maximum) {
        return rng_get_int(from_cwrap<rng_type>(self),
                           static_cast<int>(maximum));
    });
    m.def("_forward", [](py::handle self) -> unsigned int {
        return rng_forward(from_cwrap<rng_type>(self));
    });
    m.def("_get_max_int", [](py::handle self) -> unsigned int {
        return rng_get_max_int(from_cwrap<rng_type>(self));
    });
    m.def("_state_size", [](py::handle self) {
        return rng_state_size(from_cwrap<rng_type>(self));
    });
    m.def("_set_state", [](py::handle self, std::string state) {
        rng_set_state(from_cwrap<rng_type>(self), state.c_str());
    });
    m.def("_load_state", [](py::handle self, std::string filename) {
        rng_load_state(from_cwrap<rng_type>(self), filename.c_str());
    });
    m.def("_save_state", [](py::handle self, std::string filename) {
        rng_save_state(from_cwrap<rng_type>(self), filename.c_str());
    });
}
} // namespace
