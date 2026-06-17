#include <cstdint>
#include <cstdio>

#include <stdexcept>
#include <optional>
#include <tuple>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fmt/format.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_grdecl.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/FortIO.hpp>

#include <ert/util/int_vector.hpp>

#include <detail/resdata/cwrap_pybind.hpp>

namespace py = pybind11;

extern "C" {
rd_data_type *rd_type_alloc_copy_python(const rd_data_type *src_type);
}

namespace {
PYBIND11_MODULE(_kw, m) {
    register_exceptions(m);
    m.doc() = "pybind11 bindings between rd_kw.py and rd_kw.cpp";

    m.def(
        "_load_grdecl",
        [](py::handle file, std::optional<std::string> kw, bool strict,
           py::handle data_type) {
            auto *stream = from_cwrap<FILE>(file);
            auto *rd_data_type = from_cwrap<::rd_data_type>(data_type);
            if (kw.has_value())
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_fscanf_alloc_grdecl(stream, kw->c_str(),
                                              *rd_data_type, 0, strict));
            else
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_fscanf_alloc_grdecl(stream, nullptr, *rd_data_type, 0,
                                              strict));
        },
        py::return_value_policy::reference);
    m.def("_fprintf_grdecl", [](py::handle self, py::handle file) {
        auto *stream = from_cwrap<FILE>(file);
        auto *kw = from_cwrap<rd_kw_type>(self);
        rd_kw_fprintf_grdecl(kw, stream);
    });
    m.def("_fseek_grdecl", [](std::string name, bool rewind, py::handle file) {
        auto *stream = from_cwrap<FILE>(file);
        return rd_kw_grdecl_fseek_kw(name.c_str(), rewind, stream);
    });

    m.def(
        "_alloc_new",
        [](std::string name, int size, py::handle data_type) {
            auto *rd_data_type = from_cwrap<::rd_data_type>(data_type);
            if (rd_data_type == nullptr)
                throw std::invalid_argument("data_type must not be None");
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_alloc(name.c_str(), size, *rd_data_type));
        },
        py::return_value_policy::reference);
    m.def(
        "_fread_alloc",
        [](ERT::FortIO &fortio) {
            return reinterpret_cast<std::uintptr_t>(rd_kw_fread_alloc(fortio));
        },
        py::return_value_policy::reference);
    m.def(
        "_sub_copy",
        [](py::handle self, std::optional<std::string> new_kw, int offset,
           int count) {
            auto *src = from_cwrap<rd_kw_type>(self);
            if (new_kw.has_value())
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_alloc_sub_copy(src, new_kw->c_str(), offset, count));
            else
                return reinterpret_cast<std::uintptr_t>(
                    rd_kw_alloc_sub_copy(src, nullptr, offset, count));
        },
        py::return_value_policy::reference);
    m.def(
        "_copyc",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_alloc_copy(from_cwrap<rd_kw_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_slice_copyc",
        [](py::handle self, int index1, int index2, int stride) {
            return reinterpret_cast<std::uintptr_t>(rd_kw_alloc_slice_copy(
                from_cwrap<rd_kw_type>(self), index1, index2, stride));
        },
        py::return_value_policy::reference);
    m.def(
        "_global_copy",
        [](py::handle self, py::handle new_actnum) {
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_alloc_global_copy(from_cwrap<rd_kw_type>(self),
                                        from_cwrap<rd_kw_type>(new_actnum)));
        },
        py::return_value_policy::reference);
    m.def("_fprintf_data",
          [](py::handle self, std::string fmt, py::handle file) {
              rd_kw_fprintf_data(from_cwrap<rd_kw_type>(self), fmt.c_str(),
                                 from_cwrap<FILE>(file));
          });

    m.def("_get_size", [](py::handle self) {
        return rd_kw_get_size(from_cwrap<rd_kw_type>(self));
    });
    m.def("_get_fortio_size", [](py::handle self) {
        return rd_kw_fortio_size(from_cwrap<rd_kw_type>(self));
    });
    m.def("_get_type", [](py::handle self) {
        return static_cast<int>(rd_kw_get_type(from_cwrap<rd_kw_type>(self)));
    });
    m.def("_iget_char_ptr", [](py::handle self, int index) {
        return rd_kw_iget_char_ptr(from_cwrap<rd_kw_type>(self), index);
    });
    m.def("_iset_char_ptr", [](py::handle self, int index, std::string value) {
        rd_kw_iset_char_ptr(from_cwrap<rd_kw_type>(self), index, value.c_str());
    });
    m.def("_iget_string_ptr", [](py::handle self, int index) {
        return rd_kw_iget_string_ptr(from_cwrap<rd_kw_type>(self), index);
    });
    m.def("_iset_string_ptr",
          [](py::handle self, int index, std::string value) {
              rd_kw_iset_string_ptr(from_cwrap<rd_kw_type>(self), index,
                                    value.c_str());
          });
    m.def("_iget_bool", [](py::handle self, int index) {
        return rd_kw_iget_bool(from_cwrap<rd_kw_type>(self), index);
    });
    m.def("_iset_bool", [](py::handle self, int index, bool value) {
        rd_kw_iset_bool(from_cwrap<rd_kw_type>(self), index, value);
    });
    m.def(
        "_int_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_get_int_ptr(from_cwrap<rd_kw_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_float_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_get_float_ptr(from_cwrap<rd_kw_type>(self)));
        },
        py::return_value_policy::reference);
    m.def(
        "_double_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_get_double_ptr(from_cwrap<rd_kw_type>(self)));
        },
        py::return_value_policy::reference);
    m.def("_free",
          [](py::handle self) { rd_kw_free(from_cwrap<rd_kw_type>(self)); });
    m.def("_fwrite", [](py::handle self, ERT::FortIO &fortio) {
        rd_kw_fwrite(from_cwrap<rd_kw_type>(self), fortio);
    });
    m.def("_get_header", [](py::handle self) {
        return rd_kw_get_header(from_cwrap<rd_kw_type>(self));
    });
    m.def("_set_header", [](py::handle self, std::string name) {
        rd_kw_set_header_name(from_cwrap<rd_kw_type>(self), name.c_str());
    });
    m.def(
        "_get_data_type",
        [](py::handle self) {
            auto rd_kw = from_cwrap<rd_kw_type>(self);
            rd_data_type data_type = rd_kw_get_data_type(rd_kw);
            return reinterpret_cast<std::uintptr_t>(
                rd_type_alloc_copy_python(&data_type));
        },
        py::return_value_policy::reference);

    m.def("_int_sum", [](py::handle self) {
        return rd_kw_element_sum_int(from_cwrap<rd_kw_type>(self));
    });
    m.def("_float_sum", [](py::handle self) {
        return rd_kw_element_sum_float(from_cwrap<rd_kw_type>(self));
    });
    m.def("_iadd_squared", [](py::handle self, py::handle other) {
        rd_kw_inplace_add_squared(from_cwrap<rd_kw_type>(self),
                                  from_cwrap<rd_kw_type>(other));
    });
    m.def("_isqrt", [](py::handle self) {
        rd_kw_inplace_sqrt(from_cwrap<rd_kw_type>(self));
    });
    m.def("_iadd", [](py::handle self, py::handle other) {
        rd_kw_inplace_add(from_cwrap<rd_kw_type>(self),
                          from_cwrap<rd_kw_type>(other));
    });
    m.def("_imul", [](py::handle self, py::handle other) {
        rd_kw_inplace_mul(from_cwrap<rd_kw_type>(self),
                          from_cwrap<rd_kw_type>(other));
    });
    m.def("_idiv", [](py::handle self, py::handle other) {
        rd_kw_inplace_div(from_cwrap<rd_kw_type>(self),
                          from_cwrap<rd_kw_type>(other));
    });
    m.def("_isub", [](py::handle self, py::handle other) {
        rd_kw_inplace_sub(from_cwrap<rd_kw_type>(self),
                          from_cwrap<rd_kw_type>(other));
    });
    m.def("_iabs", [](py::handle self) {
        rd_kw_inplace_abs(from_cwrap<rd_kw_type>(self));
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return rd_kw_equal(from_cwrap<rd_kw_type>(self),
                           from_cwrap<rd_kw_type>(other));
    });
    m.def("_equal_numeric", [](py::handle self, py::handle other,
                               double abs_epsilon, double rel_epsilon) {
        return rd_kw_numeric_equal(from_cwrap<rd_kw_type>(self),
                                   from_cwrap<rd_kw_type>(other), abs_epsilon,
                                   rel_epsilon);
    });
    m.def("_assert_binary", [](py::handle self, py::handle other) {
        return rd_kw_size_and_numeric_type_equal(from_cwrap<rd_kw_type>(self),
                                                 from_cwrap<rd_kw_type>(other));
    });
    m.def("_scale_int", [](py::handle self, int factor) {
        rd_kw_scale_int(from_cwrap<rd_kw_type>(self), factor);
    });
    m.def("_scale_float", [](py::handle self, double factor) {
        rd_kw_scale_float_or_double(from_cwrap<rd_kw_type>(self), factor);
    });
    m.def("_shift_int", [](py::handle self, int delta) {
        rd_kw_shift_int(from_cwrap<rd_kw_type>(self), delta);
    });
    m.def("_shift_float", [](py::handle self, double delta) {
        rd_kw_shift_float_or_double(from_cwrap<rd_kw_type>(self), delta);
    });
    m.def("_copy_data", [](py::handle self, py::handle src) {
        rd_kw_memcpy_data(from_cwrap<rd_kw_type>(self),
                          from_cwrap<rd_kw_type>(src));
    });
    m.def("_set_int", [](py::handle self, int value) {
        rd_kw_scalar_set_int(from_cwrap<rd_kw_type>(self), value);
    });
    m.def("_set_float", [](py::handle self, double value) {
        rd_kw_scalar_set_float_or_double(from_cwrap<rd_kw_type>(self), value);
    });
    m.def("_max_min_int", [](py::handle self) {
        int max = 0;
        int min = 0;
        rd_kw_max_min_int(from_cwrap<rd_kw_type>(self), &max, &min);
        return std::make_tuple(max, min);
    });
    m.def("_max_min_float", [](py::handle self) {
        float max = 0;
        float min = 0;
        rd_kw_max_min_float(from_cwrap<rd_kw_type>(self), &max, &min);
        return std::make_tuple(max, min);
    });
    m.def("_max_min_double", [](py::handle self) {
        double max = 0;
        double min = 0;
        rd_kw_max_min_double(from_cwrap<rd_kw_type>(self), &max, &min);
        return std::make_tuple(max, min);
    });
    m.def("_fix_uninitialized",
          [](py::handle self, int nx, int ny, int nz, py::handle actnum) {
              rd_kw_fix_uninitialized(
                  from_cwrap<rd_kw_type>(self), nx, ny, nz,
                  int_vector_get_ptr(from_cwrap<int_vector_type>(actnum)));
          });
    m.def(
        "_create_actnum",
        [](py::handle self, float porv_limit) {
            return reinterpret_cast<std::uintptr_t>(
                rd_kw_alloc_actnum(from_cwrap<rd_kw_type>(self), porv_limit));
        },
        py::return_value_policy::reference);
    m.def("_first_different", [](py::handle self, py::handle other, int offset,
                                 double abs_epsilon, double rel_epsilon) {
        return rd_kw_first_different(from_cwrap<rd_kw_type>(self),
                                     from_cwrap<rd_kw_type>(other), offset,
                                     abs_epsilon, rel_epsilon);
    });
    m.def("_resize", [](py::handle self, int new_size) {
        rd_kw_resize(from_cwrap<rd_kw_type>(self), new_size);
    });
    m.def("_safe_div", [](py::handle self, py::handle divisor) {
        return rd_kw_inplace_safe_div(from_cwrap<rd_kw_type>(self),
                                      from_cwrap<rd_kw_type>(divisor));
    });
}
} // namespace
