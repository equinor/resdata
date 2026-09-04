#pragma once
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <optional>
#include <fmt/format.h>

#include <ert/util/util.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>

namespace rd {
/** The data stored in the kw mirror the possible rd_type_enum values that carry
  element-wise data:

    RD_INT_TYPE    -> std::vector<int>
    RD_FLOAT_TYPE  -> std::vector<float>
    RD_DOUBLE_TYPE -> std::vector<double>
    RD_CHAR_TYPE / RD_STRING_TYPE -> std::vector<std::string>
    RD_BOOL_TYPE   -> std::vector<char> (0/1 values)*/
using kw_data =
    std::variant<std::vector<int>, std::vector<float>, std::vector<double>,
                 std::vector<std::string>, std::vector<char>>;

/* the rd::KW datastructure is tightly bound to the on-disk binary format
   supplied by Eclipse, and there the number of elements is stored as a signed
   32 bit integer. Internally, size_t is used to denote size, however when
   loaded or saved to disk, the size is validated to be no larger than the
   std::numeric_limits<int>::max */

class KW {
private:
    size_t m_size;
    rd_data_type m_data_type;
    std::string m_header;
    std::optional<kw_data> m_data;
    void zero_init_data();
    static bool fread_data(rd::KW *rd_kw, ERT::FortIO &fortio);
    static std::string strip_header(const std::string &header) {
        if (header.size() > RD_STRING8_LENGTH)
            return header;
        const size_t start = header.find_first_not_of(' ');
        if (start == std::string::npos)
            return std::string();
        const size_t end = header.find_last_not_of(' ');
        return header.substr(start, end - start + 1);
    }

public:
    template <typename T> T at(size_t index) const {
        if (index >= m_size)
            throw std::invalid_argument(fmt::format(
                "Invalid index lookup. kw:{} input_index:{}   size:{}",
                m_header, index, m_size));
        if (!m_data.has_value() ||
            !std::holds_alternative<std::vector<T>>(m_data.value()))
            throw std::invalid_argument(
                fmt::format("Keyword: {} is wrong type", m_header));
        return std::get<std::vector<T>>(m_data.value())[index];
    }
    template <typename T> T &at(size_t index) {
        if (index >= m_size)
            throw std::invalid_argument(fmt::format(
                "Invalid index lookup. kw:{} input_index:{}   size:{}",
                m_header, index, m_size));
        if (!m_data.has_value() ||
            !std::holds_alternative<std::vector<T>>(m_data.value()))
            throw std::invalid_argument(
                fmt::format("Keyword: {} is wrong type", m_header));
        return std::get<std::vector<T>>(m_data.value())[index];
    }

    /* Splits @s into ctype_size()-wide chunks (space-padded/truncated),
       writing them into consecutive std::string elements starting at
       @index. This mirrors the convention of storing strings longer
       than 8 characters across consecutive RD_CHAR elements. */
    void set_string_array(size_t index, const std::string &s) {
        size_t len = ctype_size() - 1;
        size_t chunks = s.size() / len;
        if ((s.size() % len) != 0)
            chunks++;
        for (size_t i = 0; i < chunks; i++) {
            size_t offset = i * len;
            size_t length = std::min<size_t>(len, s.size() - offset);
            set_padded(index + i, s.substr(offset, length));
        }
    }

    /** Inverse of set_string_array(): concatenates @count consecutive
       ctype_size()-wide std::string elements starting at @index into a
       single string. */
    [[nodiscard]] std::string get_string_array(size_t index,
                                               size_t count) const {
        std::string result;
        size_t len = ctype_size() - 1;
        result.reserve(count * len);
        for (size_t i = 0; i < count; i++)
            result += rd::pad_spaces(at<std::string>(index + i), len);
        return result;
    }
    double as_double(size_t index) const;

    KW(rd_data_type data_type) = delete;

    KW(const std::string &header, size_t size, rd_data_type data_type)
        : m_size(size), m_data_type(data_type), m_header(strip_header(header)) {
        zero_init_data();
    }

    KW(const std::string &header, int size, rd_data_type data_type)
        : m_data_type(data_type), m_header(strip_header(header)) {
        if (size < 0)
            throw std::invalid_argument(
                fmt::format("rd_kw size was negative: {}", size));
        this->m_size = static_cast<size_t>(size);
        zero_init_data();
    }

    template <typename T>
    KW(const std::string &header, const std::vector<T> &data)
        : m_size(data.size()), m_data_type(datatype<T>::tag),
          m_header(strip_header(header)) {
        m_data = data;
    }

    KW(const std::string &header,
       const std::initializer_list<std::string> &data,
       rd_data_type data_type = RD_CHAR)
        : m_size(data.size()), m_data_type(data_type),
          m_header(strip_header(header)) {
        m_data = std::vector<std::string>(m_size, "");
        size_t i = 0;
        for (const auto &v : data)
            set_padded(i++, v);
    }

    KW(const KW &other)
        : m_size(other.size()), m_data_type(other.data_type()),
          m_header(other.header()) {
        m_data = other.m_data;
    }
    KW(const KW &other, const std::optional<std::string> &new_kw, size_t offset,
       size_t count);

    KW(const rd::KW &other, size_t index1, size_t index2, size_t stride);

    static std::unique_ptr<rd::KW> fread(ERT::FortIO &fortio);
    /* Reads a selection of elements (given by @index_map) from the data
       section of a single keyword, extracting them from m_data rather than
       the legacy raw buffer. The @kw_offset argument is the byte offset of
       the start of the keyword (i.e. its header) in the file, as stored by
       rd_file_kw. */
    static void fread_indexed_data(ERT::FortIO &fortio, offset_type kw_offset,
                                   rd_data_type data_type, int element_count,
                                   const std::vector<int> &index_map,
                                   char *io_buffer);
    static std::unique_ptr<rd::KW> make_actnum(const rd::KW *porv_kw,
                                               float porv_limit);
    static std::unique_ptr<rd::KW> global_copy(const rd::KW *src,
                                               const rd::KW *actnum);
    static std::unique_ptr<rd::KW> fread_header(ERT::FortIO &);
    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] rd_data_type data_type() const { return m_data_type; }
    [[nodiscard]] size_t ctype_size() const {
        return rd_type_get_sizeof_ctype(m_data_type);
    };
    [[nodiscard]] size_t iotype_size() const {
        return rd_type_get_sizeof_iotype(m_data_type);
    };
    void resize(size_t new_size);
    static bool fskip_data(rd_data_type data_type, const int element_count,
                           ERT::FortIO &fortio);
    static void fskip_header(ERT::FortIO &fortio);
    bool fwrite(ERT::FortIO &) const;
    std::string header() const { return m_header; };
    void set_header(std::string header) {
        this->m_header = strip_header(header);
    }
    [[nodiscard]] const std::optional<kw_data> &data() const { return m_data; }

    /* Checks that m_data holds a std::vector<T> and returns a reference to
       it. Throws std::invalid_argument otherwise (e.g. wrong type, or the
       keyword's data isn't representable by rd_kw_data_variant, such as
       RD_MESS_TYPE). */
    template <typename T>
    [[nodiscard]] const std::vector<T> &get_vector() const {
        if (!m_data.has_value() ||
            !std::holds_alternative<std::vector<T>>(m_data.value()))
            throw std::invalid_argument(
                fmt::format("Keyword: {} is wrong type", m_header));
        return std::get<std::vector<T>>(m_data.value());
    }
    template <typename T> [[nodiscard]] std::vector<T> &get_vector() {
        return const_cast<std::vector<T> &>(
            const_cast<const KW *>(this)->get_vector<T>());
    }

    /* Replaces this keyword's data with a copy of @src's data. Throws
       std::invalid_argument if the size or type doesn't match. */
    void copy_data_from(const rd::KW &src) {
        if (!rd_type_is_equal(m_data_type, src.data_type()) ||
            m_size != src.size())
            throw std::invalid_argument("type/size mismatch");
        m_data = src.m_data;
    }

    /* Sets every element of the underlying std::vector<T> in m_data to
       @value. Throws std::invalid_argument if
       there is a type mismatch between T and data_type(). */
    template <typename T> void scalar_set(T value) {
        auto &vec = get_vector<T>();
        std::fill(vec.begin(), vec.end(), value);
    }

    /** This function compares the data of two rd_kw instances, and
        returns true if the relative numerical difference is less than
        @rel_diff. */
    [[nodiscard]] bool approx_equal(const rd::KW &rd_kw2, double abs_diff,
                                    double rel_diff) const;

    template <typename T> void scale(T scale_factor) {
        auto &vec = get_vector<T>();
        std::transform(vec.begin(), vec.end(), vec.begin(),
                       [scale_factor](T x) { return x * scale_factor; });
    }
    template <typename T> void shift(T shift_value) {
        auto &vec = get_vector<T>();
        std::transform(vec.begin(), vec.end(), vec.begin(),
                       [shift_value](T x) { return x + shift_value; });
    }

    [[nodiscard]] size_t first_different(const rd::KW *kw2, size_t offset,
                                         double abs_epsilon,
                                         double rel_epsilon) const;
    [[nodiscard]] size_t fortio_size() const;
    void fwrite_data(ERT::FortIO &fortio) const;

    bool operator==(const KW &other) const;
    void operator-=(const rd::KW &sub_kw);
    bool size_and_type_equal(const rd::KW *rd_kw2) const;
    bool size_and_numeric_type_equal(const rd::KW *kw2) const;
    rd_type_enum get_type() const { return rd_type_get_type(data_type()); }
    void fix_uninitialized(int nx, int ny, int nz, const int *actnum);

    void set_padded(size_t index, const std::string &v) {
        size_t len = ctype_size() - 1;
        if (v.size() > len)
            throw std::invalid_argument(fmt::format(
                "String of length {} cannot hold input string of length {}",
                len, v.size()));
        at<std::string>(index) = rd::pad_spaces(v, len);
    }
};

/* RD_BOOL_TYPE is stored as std::vector<char> (0/1 values) in m_data - there
   is no std::vector<bool> alternative. These specializations let callers use
   at<bool>() like any other numeric type; the char/bool reference aliasing
   is safe in practice since char values are always 0 or 1, a valid bool
   bit-pattern. */
template <> inline bool KW::at<bool>(size_t index) const {
    if (index >= m_size)
        throw std::invalid_argument(
            fmt::format("Invalid index lookup. kw:{} input_index:{}   size:{}",
                        m_header, index, m_size));
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<char>>(m_data.value()))
        throw std::invalid_argument(
            fmt::format("Keyword: {} is wrong type", m_header));
    return std::get<std::vector<char>>(m_data.value())[index] != 0;
}

template <> inline bool &KW::at<bool>(size_t index) {
    if (index >= m_size)
        throw std::invalid_argument(
            fmt::format("Invalid index lookup. kw:{} input_index:{}   size:{}",
                        m_header, index, m_size));
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<char>>(m_data.value()))
        throw std::invalid_argument(
            fmt::format("Keyword: {} is wrong type", m_header));
    char &c = std::get<std::vector<char>>(m_data.value())[index];
    return reinterpret_cast<bool &>(c);
}

/* RD_BOOL_TYPE is stored as std::vector<char> in m_data, so scalar_set<bool>
   must fill the underlying std::vector<char> instead of std::vector<bool>. */
template <> inline void KW::scalar_set<bool>(bool value) {
    auto &vec = get_vector<char>();
    std::fill(vec.begin(), vec.end(), static_cast<char>(value ? 1 : 0));
}

/** Normalizes @x to a mantissa with absolute value in [0.1, 1.0) and a
 base-10 exponent, i.e. `x == mantissa * 10**exponent`. This matches
 Fortran's 'D'/'E' formatted output, which differs from the C/printf
 convention of a mantissa in [1.0, 10.0). */
inline std::tuple<double, int> normalize_scientific(double x) {
    double pow_x = std::ceil(std::log10(std::fabs(x)));
    double arg_x = x / std::pow(10.0, pow_x);
    if (x != 0.0) {
        if (std::fabs(arg_x) == 1.0) {
            arg_x *= 0.10;
            pow_x += 1;
        }
    } else {
        arg_x = 0.0;
        pow_x = 0.0;
    }
    return {arg_x, static_cast<int>(pow_x)};
}

inline size_t format_kw_element_buf(char *buf, size_t buf_size, int value) {
    int written = std::snprintf(buf, buf_size, " %11d", value);
    if (written < 0)
        throw std::runtime_error(
            "snprintf failed formatting int rd_kw element");
    return static_cast<size_t>(written);
}

inline size_t format_kw_element_buf(char *buf, size_t buf_size, float value) {
    auto [mantissa, exponent] = normalize_scientific(value);
    int written =
        std::snprintf(buf, buf_size, "  %11.8fE%+03d", mantissa, exponent);
    if (written < 0)
        throw std::runtime_error(
            "snprintf failed formatting float rd_kw element");
    return static_cast<size_t>(written);
}

inline size_t format_kw_element_buf(char *buf, size_t buf_size, double value) {
    auto [mantissa, exponent] = normalize_scientific(value);
    int written =
        std::snprintf(buf, buf_size, "  %17.14fD%+03d", mantissa, exponent);
    if (written < 0)
        throw std::runtime_error(
            "snprintf failed formatting double rd_kw element");
    return static_cast<size_t>(written);
}

inline std::string format_kw_element_fmt(size_t width) {
    return " '%-" + std::to_string(width) + "s'";
}

inline size_t format_kw_element_buf(char *buf, size_t buf_size,
                                    const char *value, const std::string &fmt) {
    int written = std::snprintf(buf, buf_size, fmt.c_str(), value);
    if (written < 0)
        throw std::runtime_error(
            "snprintf failed formatting char/string rd_kw element");
    return static_cast<size_t>(written);
}

inline std::string format_kw_element(int value) {
    char buffer[32];
    size_t len = format_kw_element_buf(buffer, sizeof(buffer), value);
    return std::string(buffer, len);
}

inline std::string format_kw_element(bool value) {
    return value ? "  T" : "  F";
}

inline std::string format_kw_element(float value) {
    char buffer[48];
    size_t len = format_kw_element_buf(buffer, sizeof(buffer), value);
    return std::string(buffer, len);
}

inline std::string format_kw_element(double value) {
    char buffer[48];
    size_t len = format_kw_element_buf(buffer, sizeof(buffer), value);
    return std::string(buffer, len);
}

inline std::string format_kw_element(const char *value, size_t width = 8) {
    std::vector<char> buffer(width + 4);
    const std::string fmt = format_kw_element_fmt(width);
    size_t len =
        format_kw_element_buf(buffer.data(), buffer.size(), value, fmt);
    return std::string(buffer.data(), len);
}
int kw_get_size(const rd::KW *);
} // namespace rd
