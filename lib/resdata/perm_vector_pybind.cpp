#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/perm_vector.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_permutation_vector, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for PermutationVector";

    m.def("_free", [](py::handle self) {
        perm_vector_free(from_cwrap<perm_vector_type>(self));
    });
    m.def("_size", [](py::handle self) {
        return perm_vector_get_size(from_cwrap<perm_vector_type>(self));
    });
    m.def("_iget", [](py::handle self, int index) {
        return perm_vector_iget(from_cwrap<perm_vector_type>(self), index);
    });
}
