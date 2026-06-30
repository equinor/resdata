#include <cstdio>
#include <cstdint>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ert/util/perm_vector.hpp>
#include <ert/util/time_t_vector.hpp>
#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

namespace {
time_t to_time_t(std::int64_t value) { return static_cast<time_t>(value); }
} // namespace

PYBIND11_MODULE(_time_vector, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings for TimeVector";

    m.def(
        "_alloc",
        [](int initial_size, std::int64_t default_value) {
            return reinterpret_cast<std::uintptr_t>(
                time_t_vector_alloc(initial_size, to_time_t(default_value)));
        },
        py::return_value_policy::reference);
    m.def(
        "_alloc_copy",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                time_t_vector_alloc_copy(from_cwrap<time_t_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_strided_copy",
        [](py::handle self, int start, int stop, int stride) {
            return reinterpret_cast<std::uintptr_t>(
                time_t_vector_alloc_strided_copy(
                    from_cwrap<time_t_vector_type>(self), start, stop, stride));
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) {
        time_t_vector_free(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_iget", [](py::handle self, int index) {
        return static_cast<std::int64_t>(
            time_t_vector_iget(from_cwrap<time_t_vector_type>(self), index));
    });
    m.def("_safe_iget", [](py::handle self, int index) {
        return static_cast<std::int64_t>(time_t_vector_safe_iget(
            from_cwrap<time_t_vector_type>(self), index));
    });
    m.def("_iset", [](py::handle self, int index, std::int64_t value) {
        time_t_vector_iset(from_cwrap<time_t_vector_type>(self), index,
                           to_time_t(value));
    });
    m.def("_size", [](py::handle self) {
        return time_t_vector_size(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_append", [](py::handle self, std::int64_t value) {
        time_t_vector_append(from_cwrap<time_t_vector_type>(self),
                             to_time_t(value));
    });
    m.def("_idel_block", [](py::handle self, int index, int block_size) {
        time_t_vector_idel_block(from_cwrap<time_t_vector_type>(self), index,
                                 block_size);
    });
    m.def("_idel", [](py::handle self, int index) {
        time_t_vector_idel(from_cwrap<time_t_vector_type>(self), index);
    });
    m.def("_pop", [](py::handle self) {
        return static_cast<std::int64_t>(
            time_t_vector_pop(from_cwrap<time_t_vector_type>(self)));
    });
    m.def("_lshift", [](py::handle self, int shift) {
        time_t_vector_lshift(from_cwrap<time_t_vector_type>(self), shift);
    });
    m.def("_rshift", [](py::handle self, int shift) {
        time_t_vector_rshift(from_cwrap<time_t_vector_type>(self), shift);
    });
    m.def("_insert", [](py::handle self, int index, std::int64_t value) {
        time_t_vector_insert(from_cwrap<time_t_vector_type>(self), index,
                             to_time_t(value));
    });
    m.def("_fprintf", [](py::handle self, py::handle stream, py::object name,
                         const std::string &fmt) {
        std::string name_value;
        const char *name_ptr = nullptr;
        if (!name.is_none()) {
            name_value = py::cast<std::string>(name);
            name_ptr = name_value.c_str();
        }
        time_t_vector_fprintf(from_cwrap<time_t_vector_type>(self),
                              from_cwrap<FILE>(stream), name_ptr, fmt.c_str());
    });
    m.def("_sort", [](py::handle self) {
        time_t_vector_sort(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_rsort", [](py::handle self) {
        time_t_vector_rsort(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_reset", [](py::handle self) {
        time_t_vector_reset(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_set_read_only", [](py::handle self, bool read_only) {
        time_t_vector_set_read_only(from_cwrap<time_t_vector_type>(self),
                                    read_only);
    });
    m.def("_get_read_only", [](py::handle self) {
        return time_t_vector_get_read_only(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_get_max", [](py::handle self) {
        return static_cast<std::int64_t>(
            time_t_vector_get_max(from_cwrap<time_t_vector_type>(self)));
    });
    m.def("_get_min", [](py::handle self) {
        return static_cast<std::int64_t>(
            time_t_vector_get_min(from_cwrap<time_t_vector_type>(self)));
    });
    m.def("_get_max_index", [](py::handle self, bool reverse) {
        return time_t_vector_get_max_index(from_cwrap<time_t_vector_type>(self),
                                           reverse);
    });
    m.def("_get_min_index", [](py::handle self, bool reverse) {
        return time_t_vector_get_min_index(from_cwrap<time_t_vector_type>(self),
                                           reverse);
    });
    m.def("_shift", [](py::handle self, std::int64_t delta) {
        time_t_vector_shift(from_cwrap<time_t_vector_type>(self),
                            to_time_t(delta));
    });
    m.def("_scale", [](py::handle self, std::int64_t factor) {
        time_t_vector_scale(from_cwrap<time_t_vector_type>(self),
                            to_time_t(factor));
    });
    m.def("_div", [](py::handle self, std::int64_t divisor) {
        time_t_vector_div(from_cwrap<time_t_vector_type>(self),
                          to_time_t(divisor));
    });
    m.def("_inplace_add", [](py::handle self, py::handle delta) {
        time_t_vector_inplace_add(from_cwrap<time_t_vector_type>(self),
                                  from_cwrap<time_t_vector_type>(delta));
    });
    m.def("_inplace_mul", [](py::handle self, py::handle factor) {
        time_t_vector_inplace_mul(from_cwrap<time_t_vector_type>(self),
                                  from_cwrap<time_t_vector_type>(factor));
    });
    m.def("_assign", [](py::handle self, std::int64_t value) {
        time_t_vector_set_all(from_cwrap<time_t_vector_type>(self),
                              to_time_t(value));
    });
    m.def("_memcpy", [](py::handle self, py::handle src) {
        time_t_vector_memcpy(from_cwrap<time_t_vector_type>(self),
                             from_cwrap<time_t_vector_type>(src));
    });
    m.def("_set_default", [](py::handle self, std::int64_t default_value) {
        time_t_vector_set_default(from_cwrap<time_t_vector_type>(self),
                                  to_time_t(default_value));
    });
    m.def("_get_default", [](py::handle self) {
        return static_cast<std::int64_t>(
            time_t_vector_get_default(from_cwrap<time_t_vector_type>(self)));
    });
    m.def("_element_size", [](py::handle self) {
        return time_t_vector_element_size(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_permute", [](py::handle self, py::handle permutation_vector) {
        time_t_vector_permute(from_cwrap<time_t_vector_type>(self),
                              from_cwrap<perm_vector_type>(permutation_vector));
    });
    m.def(
        "_sort_perm",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                time_t_vector_alloc_sort_perm(
                    from_cwrap<time_t_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_rsort_perm",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                time_t_vector_alloc_rsort_perm(
                    from_cwrap<time_t_vector_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_contains", [](py::handle self, std::int64_t value) {
        return time_t_vector_contains(from_cwrap<time_t_vector_type>(self),
                                      to_time_t(value));
    });
    m.def("_select_unique", [](py::handle self) {
        time_t_vector_select_unique(from_cwrap<time_t_vector_type>(self));
    });
    m.def("_element_sum", [](py::handle self) {
        return static_cast<std::int64_t>(
            time_t_vector_sum(from_cwrap<time_t_vector_type>(self)));
    });
    m.def("_count_equal", [](py::handle self, std::int64_t value) {
        return time_t_vector_count_equal(from_cwrap<time_t_vector_type>(self),
                                         to_time_t(value));
    });
    m.def("_init_range", [](py::handle self, std::int64_t min_value,
                            std::int64_t max_value, std::int64_t delta) {
        time_t_vector_init_range(from_cwrap<time_t_vector_type>(self),
                                 to_time_t(min_value), to_time_t(max_value),
                                 to_time_t(delta));
    });
    m.def("_init_linear", [](py::handle self, std::int64_t start_value,
                             std::int64_t end_value, int num_values) {
        return time_t_vector_init_linear(from_cwrap<time_t_vector_type>(self),
                                         to_time_t(start_value),
                                         to_time_t(end_value), num_values);
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return time_t_vector_equal(from_cwrap<time_t_vector_type>(self),
                                   from_cwrap<time_t_vector_type>(other));
    });
    m.def("_first_eq", [](py::handle self, py::handle other, int offset) {
        return time_t_vector_first_equal(from_cwrap<time_t_vector_type>(self),
                                         from_cwrap<time_t_vector_type>(other),
                                         offset);
    });
    m.def("_first_neq", [](py::handle self, py::handle other, int offset) {
        return time_t_vector_first_not_equal(
            from_cwrap<time_t_vector_type>(self),
            from_cwrap<time_t_vector_type>(other), offset);
    });
}
