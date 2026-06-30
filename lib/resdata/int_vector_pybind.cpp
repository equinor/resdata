#include <cstdio>
#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/int_vector.hpp>
#include <ert/util/perm_vector.hpp>
#include <ert/util/string_util.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_int_vector, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for IntVector";

    m.def(
        "_alloc",
        [](int initial_size, int default_value) {
            return reinterpret_cast<std::uintptr_t>(
                int_vector_alloc(initial_size, default_value));
        },
        py::return_value_policy::reference);
    m.def(
        "_create_active_list",
        [](const std::string &range_string) {
            return reinterpret_cast<std::uintptr_t>(
                string_util_alloc_active_list(range_string.c_str()));
        },
        py::return_value_policy::reference);
    m.def(
        "_create_value_list",
        [](const std::string &range_string) {
            return reinterpret_cast<std::uintptr_t>(
                string_util_alloc_value_list(range_string.c_str()));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_copy",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                int_vector_alloc_copy(from_cwrap<int_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_strided_copy",
        [](py::handle self, int start, int stop, int stride) {
            return reinterpret_cast<std::uintptr_t>(
                int_vector_alloc_strided_copy(
                    from_cwrap<int_vector_type>(self), start, stop, stride));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        int_vector_free(from_cwrap<int_vector_type>(self));
    });
    m.def("_iget", [](py::handle self, int index) {
        return int_vector_iget(from_cwrap<int_vector_type>(self), index);
    });
    m.def("_safe_iget", [](py::handle self, int index) {
        return int_vector_safe_iget(from_cwrap<int_vector_type>(self), index);
    });
    m.def("_iset", [](py::handle self, int index, int value) {
        int_vector_iset(from_cwrap<int_vector_type>(self), index, value);
    });
    m.def("_size", [](py::handle self) {
        return int_vector_size(from_cwrap<int_vector_type>(self));
    });
    m.def("_append", [](py::handle self, int value) {
        int_vector_append(from_cwrap<int_vector_type>(self), value);
    });
    m.def("_idel_block", [](py::handle self, int index, int block_size) {
        int_vector_idel_block(from_cwrap<int_vector_type>(self), index,
                              block_size);
    });
    m.def("_pop", [](py::handle self) {
        return int_vector_pop(from_cwrap<int_vector_type>(self));
    });
    m.def("_idel", [](py::handle self, int index) {
        int_vector_idel(from_cwrap<int_vector_type>(self), index);
    });
    m.def("_insert", [](py::handle self, int index, int value) {
        int_vector_insert(from_cwrap<int_vector_type>(self), index, value);
    });
    m.def("_lshift", [](py::handle self, int shift) {
        int_vector_lshift(from_cwrap<int_vector_type>(self), shift);
    });
    m.def("_rshift", [](py::handle self, int shift) {
        int_vector_rshift(from_cwrap<int_vector_type>(self), shift);
    });
    m.def("_fprintf", [](py::handle self, py::handle stream, py::object name,
                         const std::string &fmt) {
        std::string name_value;
        const char *name_ptr = nullptr;
        if (!name.is_none()) {
            name_value = py::cast<std::string>(name);
            name_ptr = name_value.c_str();
        }
        int_vector_fprintf(from_cwrap<int_vector_type>(self),
                           from_cwrap<FILE>(stream), name_ptr, fmt.c_str());
    });
    m.def("_sort", [](py::handle self) {
        int_vector_sort(from_cwrap<int_vector_type>(self));
    });
    m.def("_rsort", [](py::handle self) {
        int_vector_rsort(from_cwrap<int_vector_type>(self));
    });
    m.def("_reset", [](py::handle self) {
        int_vector_reset(from_cwrap<int_vector_type>(self));
    });
    m.def("_set_read_only", [](py::handle self, bool read_only) {
        int_vector_set_read_only(from_cwrap<int_vector_type>(self), read_only);
    });
    m.def("_get_read_only", [](py::handle self) {
        return int_vector_get_read_only(from_cwrap<int_vector_type>(self));
    });
    m.def("_get_max", [](py::handle self) {
        return int_vector_get_max(from_cwrap<int_vector_type>(self));
    });
    m.def("_get_min", [](py::handle self) {
        return int_vector_get_min(from_cwrap<int_vector_type>(self));
    });
    m.def("_get_max_index", [](py::handle self, bool reverse) {
        return int_vector_get_max_index(from_cwrap<int_vector_type>(self),
                                        reverse);
    });
    m.def("_get_min_index", [](py::handle self, bool reverse) {
        return int_vector_get_min_index(from_cwrap<int_vector_type>(self),
                                        reverse);
    });
    m.def("_shift", [](py::handle self, int delta) {
        int_vector_shift(from_cwrap<int_vector_type>(self), delta);
    });
    m.def("_scale", [](py::handle self, int factor) {
        int_vector_scale(from_cwrap<int_vector_type>(self), factor);
    });
    m.def("_div", [](py::handle self, int divisor) {
        int_vector_div(from_cwrap<int_vector_type>(self), divisor);
    });
    m.def("_inplace_add", [](py::handle self, py::handle delta) {
        int_vector_inplace_add(from_cwrap<int_vector_type>(self),
                               from_cwrap<int_vector_type>(delta));
    });
    m.def("_inplace_mul", [](py::handle self, py::handle factor) {
        int_vector_inplace_mul(from_cwrap<int_vector_type>(self),
                               from_cwrap<int_vector_type>(factor));
    });
    m.def("_assign", [](py::handle self, int value) {
        int_vector_set_all(from_cwrap<int_vector_type>(self), value);
    });
    m.def("_memcpy", [](py::handle self, py::handle src) {
        int_vector_memcpy(from_cwrap<int_vector_type>(self),
                          from_cwrap<int_vector_type>(src));
    });
    m.def("_set_default", [](py::handle self, int default_value) {
        int_vector_set_default(from_cwrap<int_vector_type>(self),
                               default_value);
    });
    m.def("_get_default", [](py::handle self) {
        return int_vector_get_default(from_cwrap<int_vector_type>(self));
    });
    m.def("_element_size", [](py::handle self) {
        return int_vector_element_size(from_cwrap<int_vector_type>(self));
    });
    m.def("_permute", [](py::handle self, py::handle permutation_vector) {
        int_vector_permute(from_cwrap<int_vector_type>(self),
                           from_cwrap<perm_vector_type>(permutation_vector));
    });
    m.def(
        "_sort_perm",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                int_vector_alloc_sort_perm(from_cwrap<int_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_rsort_perm",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                int_vector_alloc_rsort_perm(from_cwrap<int_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_contains", [](py::handle self, int value) {
        return int_vector_contains(from_cwrap<int_vector_type>(self), value);
    });
    m.def("_select_unique", [](py::handle self) {
        int_vector_select_unique(from_cwrap<int_vector_type>(self));
    });
    m.def("_element_sum", [](py::handle self) {
        return int_vector_sum(from_cwrap<int_vector_type>(self));
    });
    m.def("_get_data_ptr", [](py::handle self) {
        return reinterpret_cast<std::uintptr_t>(
            int_vector_get_ptr(from_cwrap<int_vector_type>(self)));
    });
    m.def("_count_equal", [](py::handle self, int value) {
        return int_vector_count_equal(from_cwrap<int_vector_type>(self), value);
    });
    m.def("_init_range", [](py::handle self, int min_value, int max_value,
                            int delta) {
        int_vector_init_range(from_cwrap<int_vector_type>(self), min_value,
                              max_value, delta);
    });
    m.def("_init_linear", [](py::handle self, int start_value, int end_value,
                             int num_values) {
        return int_vector_init_linear(from_cwrap<int_vector_type>(self),
                                      start_value, end_value, num_values);
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return int_vector_equal(from_cwrap<int_vector_type>(self),
                                from_cwrap<int_vector_type>(other));
    });
    m.def("_first_eq", [](py::handle self, py::handle other, int offset) {
        return int_vector_first_equal(from_cwrap<int_vector_type>(self),
                                      from_cwrap<int_vector_type>(other),
                                      offset);
    });
    m.def("_first_neq", [](py::handle self, py::handle other, int offset) {
        return int_vector_first_not_equal(from_cwrap<int_vector_type>(self),
                                          from_cwrap<int_vector_type>(other),
                                          offset);
    });
}
