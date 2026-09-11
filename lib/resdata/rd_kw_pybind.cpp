#include <algorithm>
#include <cstdint>
#include <cstdio>

#include <stdexcept>
#include <optional>
#include <tuple>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <fmt/format.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/FortIO.hpp>

#include <detail/resdata/cwrap_pybind.hpp>
#include <type_traits>
#include <variant>

namespace py = pybind11;

namespace {

class KwIterator {
public:
    explicit KwIterator(py::object self)
        : m_self(std::move(self)), m_kw(from_cwrap<rd::KW>(m_self)), m_index(0),
          m_size(m_kw->size()), m_type(m_kw->get_type()) {}

    py::object next() {
        if (m_index >= m_size)
            throw py::stop_iteration();
        size_t i = m_index++;
        switch (m_type) {
        case RD_INT_TYPE:
            return py::cast(m_kw->at<int>(i));
        case RD_FLOAT_TYPE:
            return py::cast(m_kw->at<float>(i));
        case RD_DOUBLE_TYPE:
            return py::cast(m_kw->at<double>(i));
        case RD_BOOL_TYPE:
            return py::cast(m_kw->at<bool>(i));
        case RD_CHAR_TYPE:
            return py::cast(m_kw->at<std::string>(i));
        case RD_STRING_TYPE:
            return py::cast(m_kw->at<std::string>(i));
        default:
            throw std::invalid_argument(
                "ResdataKW iteration not supported for this type");
        }
    }

private:
    // Keeps the owning ResdataKW Python object (and therefore its
    // underlying rd::KW allocation) alive for as long as the
    // iterator itself is alive.
    py::object m_self;
    rd::KW *m_kw;
    size_t m_index;
    size_t m_size;
    rd_type_enum m_type;
};

static py::list format_grdecl_tokens(py::object self) {
    rd::KW *kw = from_cwrap<rd::KW>(self);
    size_t size = kw->size();
    rd_type_enum type = kw->get_type();
    size_t string_width = kw->iotype_size();

    py::list tokens(size);
    switch (type) {
    case RD_INT_TYPE: {
        char buf[32];
        for (size_t i = 0; i < size; i++) {
            size_t len =
                rd::format_kw_element_buf(buf, sizeof(buf), kw->at<int>(i));
            tokens[i] = py::str(buf, len);
        }
        break;
    }
    case RD_FLOAT_TYPE: {
        char buf[48];
        for (size_t i = 0; i < size; i++) {
            size_t len =
                rd::format_kw_element_buf(buf, sizeof(buf), kw->at<float>(i));
            tokens[i] = py::str(buf, len);
        }
        break;
    }
    case RD_DOUBLE_TYPE: {
        char buf[48];
        for (size_t i = 0; i < size; i++) {
            size_t len =
                rd::format_kw_element_buf(buf, sizeof(buf), kw->at<double>(i));
            tokens[i] = py::str(buf, len);
        }
        break;
    }
    case RD_BOOL_TYPE:
        for (size_t i = 0; i < size; i++)
            tokens[i] = kw->at<bool>(i) ? py::str("  T") : py::str("  F");
        break;
    case RD_CHAR_TYPE: {
        const std::string fmt = rd::format_kw_element_fmt(8);
        std::vector<char> buf(8 + 4);
        for (size_t i = 0; i < size; i++) {
            const std::string value = kw->at<std::string>(i);
            size_t len = rd::format_kw_element_buf(buf.data(), buf.size(),
                                                   value.c_str(), fmt);
            tokens[i] = py::str(buf.data(), len);
        }
        break;
    }
    case RD_STRING_TYPE: {
        const std::string fmt = rd::format_kw_element_fmt(string_width);
        std::vector<char> buf(string_width + 4);
        for (size_t i = 0; i < size; i++) {
            const std::string value = kw->at<std::string>(i);
            size_t len = rd::format_kw_element_buf(buf.data(), buf.size(),
                                                   value.c_str(), fmt);
            tokens[i] = py::str(buf.data(), len);
        }
        break;
    }
    default:
        throw std::invalid_argument(
            "ResdataKW formatted iteration not supported for this type");
    }
    return tokens;
}

PYBIND11_MODULE(_kw, m) {
    py::class_<KwIterator>(m, "_KwIterator")
        .def("__iter__", [](KwIterator &it) -> KwIterator & { return it; })
        .def("__next__", &KwIterator::next);
    m.def("_iter", [](py::object self) { return KwIterator(std::move(self)); });
    m.def("_format_grdecl_tokens", &format_grdecl_tokens);
    register_exceptions(m);
    m.doc() = "pybind11 bindings between rd_kw.py and rd_kw.cpp";

    m.def(
        "_alloc_new",
        [](std::string name, size_t size, py::handle data_type) {
            auto *rd_data_type = from_cwrap<::rd_data_type>(data_type);
            if (rd_data_type == nullptr)
                throw std::invalid_argument("data_type must not be None");
            return to_capsule(new rd::KW{name, size, *rd_data_type});
        },
        py::return_value_policy::reference);
    m.def(
        "_fread_alloc",
        [](ERT::FortIO &fortio) {
            return to_capsule(rd::KW::fread(fortio).release());
        },
        py::return_value_policy::reference);
    m.def(
        "_sub_copy",
        [](py::handle self, std::optional<std::string> new_kw, size_t offset,
           py::int_ count) {
            auto *src = from_cwrap<rd::KW>(self);

            size_t count_size;
            if (count < py::int_{0})
                count_size = py::int_{src->size() - offset};
            else
                count_size = count.cast<size_t>();

            return to_capsule(new rd::KW{*src, new_kw, offset, count_size});
        },
        py::return_value_policy::reference);
    m.def(
        "_copyc",
        [](py::handle self) {
            return to_capsule(new rd::KW{*from_cwrap<rd::KW>(self)});
        },
        py::return_value_policy::reference);
    m.def(
        "_slice_copyc",
        [](py::handle self, py::int_ index1, py::int_ index2, size_t stride) {
            if (index1 < py::int_{0})
                index1 = 0;
            if (index2 < py::int_{0})
                index2 = 0;
            return to_capsule(new rd::KW{*from_cwrap<rd::KW>(self),
                                         index1.cast<size_t>(),
                                         index2.cast<size_t>(), stride});
        },
        py::return_value_policy::reference);
    m.def(
        "_global_copy",
        [](py::handle self, py::handle new_actnum) {
            return to_capsule(
                rd::KW::global_copy(from_cwrap<rd::KW>(self),
                                    from_cwrap<rd::KW>(new_actnum))
                    .release());
        },
        py::return_value_policy::reference);

    m.def("_get_size",
          [](py::handle self) { return from_cwrap<rd::KW>(self)->size(); });
    m.def("_get_fortio_size", [](py::handle self) {
        return from_cwrap<rd::KW>(self)->fortio_size();
    });
    m.def("_get_type", [](py::handle self) {
        return static_cast<int>(from_cwrap<rd::KW>(self)->get_type());
    });
    m.def("_iget_char_ptr", [](py::handle self, size_t index) {
        return from_cwrap<rd::KW>(self)->at<std::string>(index);
    });
    m.def("_iset_char_ptr",
          [](py::handle self, size_t index, std::string value) {
              from_cwrap<rd::KW>(self)->set_string_array(index, value);
          });
    m.def("_iget_string_ptr", [](py::handle self, size_t index) {
        return from_cwrap<rd::KW>(self)->at<std::string>(index);
    });
    m.def("_iset_string_ptr",
          [](py::handle self, int index, std::string value) {
              auto kw = from_cwrap<rd::KW>(self);
              kw->set_padded(index, value);
          });
    m.def("_iget_bool", [](py::handle self, size_t index) {
        return (from_cwrap<rd::KW>(self))->at<bool>(index);
    });
    m.def("_iset_bool", [](py::handle self, size_t index, bool value) {
        (from_cwrap<rd::KW>(self))->at<bool>(index) = value;
    });
    m.def(
        "_int_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                from_cwrap<rd::KW>(self)->get_vector<int>().data());
        },
        py::return_value_policy::reference);
    m.def(
        "_float_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                from_cwrap<rd::KW>(self)->get_vector<float>().data());
        },
        py::return_value_policy::reference);
    m.def(
        "_double_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                from_cwrap<rd::KW>(self)->get_vector<double>().data());
        },
        py::return_value_policy::reference);
    m.def(
        "_bool_ptr",
        [](py::handle self) {
            return reinterpret_cast<std::uintptr_t>(
                from_cwrap<rd::KW>(self)->get_vector<char>().data());
        },
        py::return_value_policy::reference);
    m.def("_free", [](py::handle self) { delete from_cwrap<rd::KW>(self); });
    m.def("_fwrite", [](py::handle self, ERT::FortIO &fortio) {
        from_cwrap<rd::KW>(self)->fwrite(fortio);
    });
    m.def("_get_header",
          [](py::handle self) { return from_cwrap<rd::KW>(self)->header(); });
    m.def("_set_header", [](py::handle self, std::string name) {
        from_cwrap<rd::KW>(self)->set_header(name);
    });
    m.def(
        "_get_data_type",
        [](py::handle self) {
            auto rd_kw = from_cwrap<rd::KW>(self);
            rd_data_type data_type = rd_kw->data_type();
            return to_capsule(new rd_data_type(data_type));
        },
        py::return_value_policy::reference);

    m.def("_iadd_squared", [](py::handle self, py::handle other) {
        auto target_kw = from_cwrap<rd::KW>(self);
        auto add_kw = from_cwrap<rd::KW>(other);
        std::visit(
            [&](auto &&target_alt) {
                using T =
                    typename std::decay_t<decltype(target_alt)>::value_type;
                if constexpr (std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, double>) {
                    if (!target_kw->size_and_numeric_type_equal(add_kw))
                        throw std::invalid_argument("type/size  mismatch");
                    auto &target_data = target_kw->get_vector<T>();
                    const auto &add_data = add_kw->get_vector<T>();
                    for (size_t i = 0; i < target_data.size(); i++)
                        target_data[i] += add_data[i] * add_data[i];
                } else
                    throw std::invalid_argument(
                        fmt::format("inplace add not implemented for type:{}",
                                    rd_type_name(target_kw->data_type())));
            },
            target_kw->data().value());
    });
    m.def("_isqrt", [](py::handle self) {
        auto kw = from_cwrap<rd::KW>(self);
        std::visit(
            [&](auto &&alt) {
                using T = typename std::decay_t<decltype(alt)>::value_type;
                if constexpr (std::is_same_v<T, int>) {
                    auto &data = kw->get_vector<T>();
                    for (auto &value : data)
                        value = static_cast<int>(std::round(std::sqrt(value)));
                } else if constexpr (std::is_same_v<T, float> ||
                                     std::is_same_v<T, double>) {
                    auto &data = kw->get_vector<T>();
                    for (auto &value : data)
                        value = std::sqrt(value);
                } else
                    throw std::invalid_argument(
                        fmt::format("inplace sqrt not implemented for type:{}",
                                    rd_type_name(kw->data_type())));
            },
            kw->data().value());
    });
    m.def("_iadd", [](py::handle self, py::handle other) {
        auto target_kw = from_cwrap<rd::KW>(self);
        auto add_kw = from_cwrap<rd::KW>(other);
        std::visit(
            [&](auto &&target_alt) {
                using T =
                    typename std::decay_t<decltype(target_alt)>::value_type;
                if constexpr (std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, double>) {
                    if (!target_kw->size_and_numeric_type_equal(add_kw))
                        throw std::invalid_argument("type/size  mismatch");
                    auto &target_data = target_kw->get_vector<T>();
                    const auto &add_data = add_kw->get_vector<T>();
                    for (size_t i = 0; i < target_data.size(); i++)
                        target_data[i] += add_data[i];
                } else
                    throw std::invalid_argument(
                        fmt::format("inplace add not implemented for type:{}",
                                    rd_type_name(target_kw->data_type())));
            },
            target_kw->data().value());
    });
    m.def("_imul", [](py::handle self, py::handle other) {
        auto target_kw = from_cwrap<rd::KW>(self);
        auto mul_kw = from_cwrap<rd::KW>(other);

        std::visit(
            [&](auto &&target_alt) {
                using T =
                    typename std::decay_t<decltype(target_alt)>::value_type;
                if constexpr (std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, double>) {
                    if (!target_kw->size_and_numeric_type_equal(mul_kw))
                        throw std::invalid_argument("type/size  mismatch");
                    auto &target_data = target_kw->get_vector<T>();
                    const auto &mul_data = mul_kw->get_vector<T>();
                    for (size_t i = 0; i < target_data.size(); i++)
                        target_data[i] *= mul_data[i];
                } else
                    throw std::invalid_argument(
                        fmt::format("inplace mul not implemented for type:{}",
                                    rd_type_name(target_kw->data_type())));
            },
            target_kw->data().value());
    });
    m.def("_idiv", [](py::handle self, py::handle other) {
        auto target_kw = from_cwrap<rd::KW>(self);
        auto div_kw = from_cwrap<rd::KW>(other);
        std::visit(
            [&](auto &&target_alt) {
                using T =
                    typename std::decay_t<decltype(target_alt)>::value_type;
                if constexpr (std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, double>) {
                    if (!target_kw->size_and_numeric_type_equal(div_kw))
                        throw std::invalid_argument("type/size  mismatch");
                    auto &target_data = target_kw->get_vector<T>();
                    const auto &div_data = div_kw->get_vector<T>();
                    for (size_t i = 0; i < target_data.size(); i++)
                        target_data[i] /= div_data[i];
                } else
                    throw std::invalid_argument(
                        fmt::format("inplace div not implemented for type:{}",
                                    rd_type_name(target_kw->data_type())));
            },
            target_kw->data().value());
    });
    m.def("_isub", [](py::handle self, py::handle other) {
        *from_cwrap<rd::KW>(self) -= *from_cwrap<rd::KW>(other);
    });
    m.def("_iabs", [](py::handle self) {
        auto kw = from_cwrap<rd::KW>(self);
        std::visit(
            [&](auto &&alt) {
                using T = typename std::decay_t<decltype(alt)>::value_type;
                if constexpr (std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, double>) {
                    auto &data = kw->get_vector<T>();
                    for (auto &value : data)
                        value = std::abs(value);
                } else
                    throw std::invalid_argument(
                        fmt::format("inplace abs not implemented for type:{}",
                                    rd_type_name(kw->data_type())));
            },
            kw->data().value());
    });
    m.def("_equal", [](py::handle self, py::handle other) {
        return *from_cwrap<rd::KW>(self) == *from_cwrap<rd::KW>(other);
    });
    m.def("_equal_numeric", [](py::handle self, py::handle other,
                               double abs_epsilon, double rel_epsilon) {
        return from_cwrap<rd::KW>(self)->approx_equal(
            *from_cwrap<rd::KW>(other), abs_epsilon, rel_epsilon);
    });
    m.def("_assert_binary", [](py::handle self, py::handle other) {
        return from_cwrap<rd::KW>(self)->size_and_numeric_type_equal(
            from_cwrap<rd::KW>(other));
    });
    m.def("_scale_int", [](py::handle self, int factor) {
        from_cwrap<rd::KW>(self)->scale<int>(factor);
    });
    m.def("_scale_float", [](py::handle self, double factor) {
        rd::KW *rd_kw = from_cwrap<rd::KW>(self);
        rd_type_enum rd_type = rd_kw->get_type();
        if (rd_type == RD_FLOAT_TYPE)
            rd_kw->scale<float>(factor);
        else if (rd_type == RD_DOUBLE_TYPE)
            rd_kw->scale<double>(factor);
        else
            throw std::invalid_argument("wrong type");
    });
    m.def("_shift_int", [](py::handle self, int delta) {
        from_cwrap<rd::KW>(self)->shift<int>(delta);
    });
    m.def("_shift_float", [](py::handle self, double delta) {
        auto rd_kw = from_cwrap<rd::KW>(self);
        rd_type_enum rd_type = rd_kw->get_type();
        if (rd_type == RD_FLOAT_TYPE)
            rd_kw->shift<float>(static_cast<float>(delta));
        else if (rd_type == RD_DOUBLE_TYPE)
            rd_kw->shift<double>(delta);
        else
            throw std::invalid_argument("wrong type");
    });
    m.def("_copy_data", [](py::handle self, py::handle src) {
        from_cwrap<rd::KW>(self)->copy_data_from(*from_cwrap<rd::KW>(src));
    });
    m.def("_set_int", [](py::handle self, int value) {
        from_cwrap<rd::KW>(self)->scalar_set<int>(value);
    });
    m.def("_set_float", [](py::handle self, double value) {
        auto rd_kw = from_cwrap<rd::KW>(self);
        rd_type_enum rd_type = rd_kw->get_type();
        if (rd_type == RD_FLOAT_TYPE)
            rd_kw->scalar_set<float>((float)value);
        else if (rd_type == RD_DOUBLE_TYPE)
            rd_kw->scalar_set<double>(value);
        else
            throw std::invalid_argument("wrong type");
    });
    m.def("_max_min_int", [](py::handle self) {
        auto &data = from_cwrap<rd::KW>(self)->get_vector<int>();
        auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());

        return std::make_tuple(*max_it, *min_it);
    });
    m.def("_max_min_float", [](py::handle self) {
        auto &data = from_cwrap<rd::KW>(self)->get_vector<float>();
        auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());

        return std::make_tuple(*max_it, *min_it);
    });
    m.def("_max_min_double", [](py::handle self) {
        auto &data = from_cwrap<rd::KW>(self)->get_vector<double>();
        auto [min_it, max_it] = std::minmax_element(data.begin(), data.end());
        return std::make_tuple(*max_it, *min_it);
    });
    m.def(
        "_fix_uninitialized",
        [](py::handle self, int nx, int ny, int nz,
           py::array_t<int, py::array::c_style | py::array::forcecast> actnum) {
            if (nx < 0 || ny < 0 || nz < 0)
                throw std::invalid_argument(
                    "nx, ny and nz must be non-negative");

            auto required_size = static_cast<py::ssize_t>(nx) * ny * nz;
            if (actnum.size() < required_size)
                throw std::invalid_argument(fmt::format(
                    "actnum has {} elements, but nx*ny*nz={} were expected",
                    actnum.size(), required_size));

            from_cwrap<rd::KW>(self)->fix_uninitialized(nx, ny, nz,
                                                        actnum.data());
        });
    m.def(
        "_create_actnum",
        [](py::handle self, float porv_limit) {
            return to_capsule(
                rd::KW::make_actnum(from_cwrap<rd::KW>(self), porv_limit)
                    .release());
        },
        py::return_value_policy::reference);
    m.def("_first_different",
          [](py::handle self, py::handle other, size_t offset,
             double abs_epsilon, double rel_epsilon) {
              return from_cwrap<rd::KW>(self)->first_different(
                  from_cwrap<rd::KW>(other), offset, abs_epsilon, rel_epsilon);
          });
    m.def("_resize", [](py::handle self, size_t new_size) {
        from_cwrap<rd::KW>(self)->resize(new_size);
    });
    m.def("_safe_div", [](py::handle self, py::handle divisor) {
        auto target_kw = from_cwrap<rd::KW>(self);
        auto divisor_kw = from_cwrap<rd::KW>(divisor);
        if (target_kw->get_type() != RD_FLOAT_TYPE)
            return false;

        if (divisor_kw->get_type() != RD_INT_TYPE)
            return false;

        auto &target_data = target_kw->get_vector<float>();
        const auto &div_data = divisor_kw->get_vector<int>();
        for (size_t i = 0; i < target_kw->size(); i++) {
            if (div_data[i] != 0)
                target_data[i] /= static_cast<float>(div_data[i]);
        }

        return true;
    });
}
} // namespace
