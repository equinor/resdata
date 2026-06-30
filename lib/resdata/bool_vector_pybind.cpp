#include <cstdio>
#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/bool_vector.hpp>
#include <ert/util/perm_vector.hpp>
#include <ert/util/string_util.hpp>
#include <ert/util/type_vector_functions.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_bool_vector, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for BoolVector";

    m.def(
        "_alloc",
        [](int initial_size, bool default_value) {
            return reinterpret_cast<std::uintptr_t>(
                bool_vector_alloc(initial_size, default_value));
        },
        py::return_value_policy::reference);
    m.def(
        "_create_active_mask",
        [](const std::string &range_string) {
            return reinterpret_cast<std::uintptr_t>(
                string_util_alloc_active_mask(range_string.c_str()));
        },
        py::return_value_policy::reference);
    m.def(
        "_active_list",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                bool_vector_alloc_active_list(
                    from_cwrap<bool_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_copy",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                bool_vector_alloc_copy(from_cwrap<bool_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_update_active_mask", [](const std::string &range_string,
                                    py::handle active_mask) {
        return string_util_update_active_mask(
            range_string.c_str(), from_cwrap<bool_vector_type>(active_mask));
    });
    m.def(
        "_strided_copy",
        [](py::handle self, int start, int stop, int stride) {
            return reinterpret_cast<std::uintptr_t>(
                bool_vector_alloc_strided_copy(
                    from_cwrap<bool_vector_type>(self), start, stop, stride));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        bool_vector_free(from_cwrap<bool_vector_type>(self));
    });
    m.def("_iget", [](py::handle self, int index) {
        return bool_vector_iget(from_cwrap<bool_vector_type>(self), index);
    });
    m.def("_safe_iget", [](py::handle self, int index) {
        return bool_vector_safe_iget(from_cwrap<bool_vector_type>(self), index);
    });
    m.def("_iset", [](py::handle self, int index, bool value) {
        bool_vector_iset(from_cwrap<bool_vector_type>(self), index, value);
    });
    m.def("_size", [](py::handle self) {
        return bool_vector_size(from_cwrap<bool_vector_type>(self));
    });
    m.def("_append", [](py::handle self, bool value) {
        bool_vector_append(from_cwrap<bool_vector_type>(self), value);
    });
    m.def("_idel_block", [](py::handle self, int index, int block_size) {
        bool_vector_idel_block(from_cwrap<bool_vector_type>(self), index,
                               block_size);
    });
    m.def("_idel", [](py::handle self, int index) {
        bool_vector_idel(from_cwrap<bool_vector_type>(self), index);
    });
    m.def("_pop", [](py::handle self) {
        return bool_vector_pop(from_cwrap<bool_vector_type>(self));
    });
    m.def("_lshift", [](py::handle self, int shift) {
        bool_vector_lshift(from_cwrap<bool_vector_type>(self), shift);
    });
    m.def("_rshift", [](py::handle self, int shift) {
        bool_vector_rshift(from_cwrap<bool_vector_type>(self), shift);
    });
    m.def("_insert", [](py::handle self, int index, bool value) {
        bool_vector_insert(from_cwrap<bool_vector_type>(self), index, value);
    });
    m.def("_fprintf", [](py::handle self, py::handle stream, py::object name,
                         const std::string &fmt) {
        std::string name_value;
        const char *name_ptr = nullptr;
        if (!name.is_none()) {
            name_value = py::cast<std::string>(name);
            name_ptr = name_value.c_str();
        }
        bool_vector_fprintf(from_cwrap<bool_vector_type>(self),
                            from_cwrap<FILE>(stream), name_ptr, fmt.c_str());
    });
    m.def("_sort", [](py::handle self) {
        bool_vector_sort(from_cwrap<bool_vector_type>(self));
    });
    m.def("_rsort", [](py::handle self) {
        bool_vector_rsort(from_cwrap<bool_vector_type>(self));
    });
    m.def("_reset", [](py::handle self) {
        bool_vector_reset(from_cwrap<bool_vector_type>(self));
    });
    m.def("_set_read_only", [](py::handle self, bool read_only) {
        bool_vector_set_read_only(from_cwrap<bool_vector_type>(self),
                                  read_only);
    });
    m.def("_get_read_only", [](py::handle self) {
        return bool_vector_get_read_only(from_cwrap<bool_vector_type>(self));
    });
    m.def("_get_max", [](py::handle self) {
        return bool_vector_get_max(from_cwrap<bool_vector_type>(self));
    });
    m.def("_get_min", [](py::handle self) {
        return bool_vector_get_min(from_cwrap<bool_vector_type>(self));
    });
    m.def("_get_max_index", [](py::handle self, bool reverse) {
        return bool_vector_get_max_index(from_cwrap<bool_vector_type>(self),
                                         reverse);
    });
    m.def("_get_min_index", [](py::handle self, bool reverse) {
        return bool_vector_get_min_index(from_cwrap<bool_vector_type>(self),
                                         reverse);
    });
    m.def("_shift", [](py::handle self, bool delta) {
        bool_vector_shift(from_cwrap<bool_vector_type>(self), delta);
    });
    m.def("_scale", [](py::handle self, bool factor) {
        bool_vector_scale(from_cwrap<bool_vector_type>(self), factor);
    });
    m.def("_div", [](py::handle self, bool divisor) {
        bool_vector_div(from_cwrap<bool_vector_type>(self), divisor);
    });
    m.def("_inplace_add", [](py::handle self, py::handle delta) {
        bool_vector_inplace_add(from_cwrap<bool_vector_type>(self),
                                from_cwrap<bool_vector_type>(delta));
    });
    m.def("_inplace_mul", [](py::handle self, py::handle factor) {
        bool_vector_inplace_mul(from_cwrap<bool_vector_type>(self),
                                from_cwrap<bool_vector_type>(factor));
    });
    m.def("_assign", [](py::handle self, bool value) {
        bool_vector_set_all(from_cwrap<bool_vector_type>(self), value);
    });
    m.def("_memcpy", [](py::handle self, py::handle src) {
        bool_vector_memcpy(from_cwrap<bool_vector_type>(self),
                           from_cwrap<bool_vector_type>(src));
    });
    m.def("_set_default", [](py::handle self, bool default_value) {
        bool_vector_set_default(from_cwrap<bool_vector_type>(self),
                                default_value);
    });
    m.def("_get_default", [](py::handle self) {
        return bool_vector_get_default(from_cwrap<bool_vector_type>(self));
    });
    m.def("_element_size", [](py::handle self) {
        return bool_vector_element_size(from_cwrap<bool_vector_type>(self));
    });
    m.def("_permute", [](py::handle self, py::handle permutation_vector) {
        bool_vector_permute(from_cwrap<bool_vector_type>(self),
                            from_cwrap<perm_vector_type>(permutation_vector));
    });
    m.def(
        "_sort_perm",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                bool_vector_alloc_sort_perm(
                    from_cwrap<bool_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_rsort_perm",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                bool_vector_alloc_rsort_perm(
                    from_cwrap<bool_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_contains", [](py::handle self, bool value) {
        return bool_vector_contains(from_cwrap<bool_vector_type>(self), value);
    });
    m.def("_select_unique", [](py::handle self) {
        bool_vector_select_unique(from_cwrap<bool_vector_type>(self));
    });
    m.def("_element_sum", [](py::handle self) {
        return bool_vector_sum(from_cwrap<bool_vector_type>(self));
    });
    m.def("_get_data_ptr", [](py::handle self) {
        return reinterpret_cast<std::uintptr_t>(
            bool_vector_get_ptr(from_cwrap<bool_vector_type>(self)));
    });
    m.def("_count_equal", [](py::handle self, bool value) {
        return bool_vector_count_equal(from_cwrap<bool_vector_type>(self),
                                       value);
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return bool_vector_equal(from_cwrap<bool_vector_type>(self),
                                 from_cwrap<bool_vector_type>(other));
    });
    m.def("_first_eq", [](py::handle self, py::handle other, int offset) {
        return bool_vector_first_equal(from_cwrap<bool_vector_type>(self),
                                       from_cwrap<bool_vector_type>(other),
                                       offset);
    });
    m.def("_first_neq", [](py::handle self, py::handle other, int offset) {
        return bool_vector_first_not_equal(
            from_cwrap<bool_vector_type>(self),
            from_cwrap<bool_vector_type>(other), offset);
    });
}
