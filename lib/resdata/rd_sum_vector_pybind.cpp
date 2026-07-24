#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <resdata/rd_sum_vector.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_rd_sum_keyword_vector, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for SummaryKeyWordVector";

    m.def(
        "_alloc",
        [](py::handle rd_sum, bool add_keywords) {
            return reinterpret_cast<std::uintptr_t>(rd_sum_vector_alloc(
                from_cwrap<rd_sum_type>(rd_sum), add_keywords));
        },
        py::return_value_policy::reference);

    m.def(
        "_alloc_copy",
        [](py::handle self, py::handle rd_sum) {
            return reinterpret_cast<std::uintptr_t>(
                rd_sum_vector_alloc_layout_copy(
                    from_cwrap<rd_sum_vector_type>(self),
                    from_cwrap<rd_sum_type>(rd_sum)));
        },
        py::return_value_policy::reference);

    m.def("_free", [](py::handle self) {
        rd_sum_vector_free(from_cwrap<rd_sum_vector_type>(self));
    });
    m.def("_add", [](py::handle self, const std::string &key) {
        return rd_sum_vector_add_key(from_cwrap<rd_sum_vector_type>(self),
                                     key.c_str());
    });
    m.def("_add_multiple", [](py::handle self, const std::string &pattern) {
        rd_sum_vector_add_keys(from_cwrap<rd_sum_vector_type>(self),
                               pattern.c_str());
    });
    m.def("_get_size", [](py::handle self) {
        return rd_sum_vector_get_size(from_cwrap<rd_sum_vector_type>(self));
    });
    m.def("_iget_key", [](py::handle self, int index) {
        return std::string(rd_sum_vector_iget_key(
            from_cwrap<rd_sum_vector_type>(self), index));
    });
}
