#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <algorithm>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include <ert/util/util.hpp>

#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>

namespace {
[[noreturn]] void throw_invalid_index(const std::string &header, size_t index,
                                      size_t size) {
    throw std::invalid_argument(
        fmt::format("Invalid index lookup. kw:{} input_index:{}   size:{}",
                    header, index, size));
}

[[noreturn]] void throw_wrong_type(const std::string &header) {
    throw std::invalid_argument(
        fmt::format("Keyword: {} is wrong type", header));
}
} // namespace

template <typename T> T rd::KW::at(size_t index) const {
    if (index >= m_size)
        throw_invalid_index(m_name, index, m_size);
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<T>>(m_data.value()))
        throw_wrong_type(m_name);
    return std::get<std::vector<T>>(m_data.value())[index];
}

template <typename T> T &rd::KW::at(size_t index) {
    if (index >= m_size)
        throw_invalid_index(m_name, index, m_size);
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<T>>(m_data.value()))
        throw_wrong_type(m_name);
    return std::get<std::vector<T>>(m_data.value())[index];
}

template <> bool rd::KW::at<bool>(size_t index) const {
    if (index >= m_size)
        throw_invalid_index(m_name, index, m_size);
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<char>>(m_data.value()))
        throw_wrong_type(m_name);
    return std::get<std::vector<char>>(m_data.value())[index] != 0;
}

template <typename T> const std::vector<T> &rd::KW::get_vector() const {
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<T>>(m_data.value()))
        throw_wrong_type(m_name);
    return std::get<std::vector<T>>(m_data.value());
}

/* rd::KW stores its elements in kw_data, so at() and get_vector() are only
   ever used with the alternatives of that variant. */
#define INSTANTIATE_KW_ACCESSORS(T)                                            \
    template T rd::KW::at<T>(size_t) const;                                    \
    template T &rd::KW::at<T>(size_t);                                         \
    template const std::vector<T> &rd::KW::get_vector<T>() const;

INSTANTIATE_KW_ACCESSORS(int)
INSTANTIATE_KW_ACCESSORS(float)
INSTANTIATE_KW_ACCESSORS(double)
INSTANTIATE_KW_ACCESSORS(char)
INSTANTIATE_KW_ACCESSORS(std::string)
#undef INSTANTIATE_KW_ACCESSORS

rd::KW::KW(const std::string &header, int size, rd_data_type data_type)
    : m_data_type(data_type), m_name(strip_name(header)) {
    if (size < 0)
        throw std::invalid_argument(
            fmt::format("rd_kw size was negative: {}", size));
    this->m_size = static_cast<size_t>(size);
    zero_init_data();
}

void rd::KW::set_bool(size_t index, bool value) {
    if (index >= m_size)
        throw_invalid_index(m_name, index, m_size);
    if (!m_data.has_value() ||
        !std::holds_alternative<std::vector<char>>(m_data.value()))
        throw_wrong_type(m_name);
    std::get<std::vector<char>>(m_data.value())[index] =
        static_cast<char>(value ? 1 : 0);
}

void rd::KW::set_padded(size_t index, const std::string &v) {
    size_t len = ctype_size() - 1;
    if (v.size() > len)
        throw std::invalid_argument(fmt::format(
            "String of length {} cannot hold input string of length {}", len,
            v.size()));
    at<std::string>(index) = rd::pad_spaces(v, len);
}

double rd::KW::as_double(size_t index) const {
    if (rd_type_is_float(m_data_type)) {
        return static_cast<double>(this->at<float>(index));
    } else if (rd_type_is_double(m_data_type)) {
        return this->at<double>(index);
    } else if (rd_type_is_int(m_data_type)) {
        return static_cast<double>(this->at<int>(index));
    } else
        throw std::invalid_argument("cannot be converted to double");
}

/*
  Character data in restart format files comes as an array of fixed-length
  string. Each of these strings is 8 characters long. The type name,
  i.e. 'REAL', 'INTE', ... , come as 4 character strings.
*/
#define RD_KW_HEADER_DATA_SIZE RD_STRING8_LENGTH + RD_TYPE_LENGTH + 4
#define RD_KW_HEADER_FORTIO_SIZE RD_KW_HEADER_DATA_SIZE + 8

/* For some peculiar reason the keyword data is written in blocks, all
   numeric data is in blocks of 1000 elements, and character data is
   in blocks of 105 elements.
*/

#define BLOCKSIZE_NUMERIC 1000
#define BLOCKSIZE_CHAR 105

/* When writing formatted data, the data comes in columns, with a
   certain number of elements in each row, i.e. four columns for float
   data:

   0.000   0.000   0.000   0.000
   0.000   0.000   0.000   0.000
   0.000   0.000   0.000   0.000
   ....

   These #define symbols define the number of columns for the
   different datatypes.
*/
#define COLUMNS_CHAR 7
#define COLUMNS_FLOAT 4
#define COLUMNS_DOUBLE 3
#define COLUMNS_INT 6
#define COLUMNS_MESSAGE 1
#define COLUMNS_BOOL 25

/* Format string used when writing a formatted header. */
#define WRITE_HEADER_FMT " '%-8s' %11d '%-4s'\n"

/* Format string used when reading and writing formatted
   files. Observe the following about these format strings:

    1. The format string for reading double contains two '%'
       identifiers, that is because doubles are read by parsing a
       prefix and power separately.

    2. For both double and float the write format contains two '%'
       characters - that is because the values are split in a prefix
       and a power prior to writing - see the function
       __fprintf_scientific().

    3. The logical type involves converting back and forth between 'T'
       and 'F' and internal logical representation. The format strings
       are therefore for reading/writing a character.

*/

#define READ_FMT_CHAR "%8c"
#define READ_FMT_FLOAT "%gE"
#define READ_FMT_INT "%d"
#define READ_FMT_MESS "%8c"
#define READ_FMT_BOOL "  %c"
#define READ_FMT_DOUBLE "%lgD%d"

/* The boolean type is not a native type which can be uniquely
   identified between Fortran, C, formatted and unformatted
   files:

    o In the formatted files the characters BOOL_TRUE_CHAR and
      BOOL_FALSE_CHAR are used to represent true and false values
      repsectively.

    o In the unformatted files the boolean values are
      represented as integers with the values RD_BOOL_TRUE_INT and
      RD_BOOL_FALSE_INT respectively.

   Internally in an rd_kw instance boolean values are represented as
   char (NOT bool). */

// For formatted files:
#define BOOL_TRUE_CHAR 'T'
#define BOOL_FALSE_CHAR 'F'

static std::string read_fmt_string(const rd_data_type rd_type) {
    return fmt::format("%{}c", rd_type_get_sizeof_iotype(rd_type));
}

static std::string read_fmt(const rd_data_type data_type) {
    switch (rd_type_get_type(data_type)) {
    case (RD_CHAR_TYPE):
        return READ_FMT_CHAR;
    case (RD_INT_TYPE):
        return READ_FMT_INT;
    case (RD_FLOAT_TYPE):
        return READ_FMT_FLOAT;
    case (RD_DOUBLE_TYPE):
        return READ_FMT_DOUBLE;
    case (RD_BOOL_TYPE):
        return READ_FMT_BOOL;
    case (RD_MESS_TYPE):
        return READ_FMT_MESS;
    case (RD_STRING_TYPE):
        return read_fmt_string(data_type);
    default:
        throw std::invalid_argument(
            fmt::format("invalid rd_type: {}", rd_type_name(data_type)));
    }
}

static size_t get_blocksize(rd_data_type data_type) {
    if (rd_type_is_alpha(data_type))
        return BLOCKSIZE_CHAR;

    return BLOCKSIZE_NUMERIC;
}

static size_t get_columns(const rd_data_type data_type) {
    switch (rd_type_get_type(data_type)) {
    case (RD_CHAR_TYPE):
        return COLUMNS_CHAR;
    case (RD_INT_TYPE):
        return COLUMNS_INT;
    case (RD_FLOAT_TYPE):
        return COLUMNS_FLOAT;
    case (RD_DOUBLE_TYPE):
        return COLUMNS_DOUBLE;
    case (RD_BOOL_TYPE):
        return COLUMNS_BOOL;
    case (RD_MESS_TYPE):
        return COLUMNS_MESSAGE;
    case (RD_STRING_TYPE):
        return COLUMNS_CHAR; // TODO: Is this correct?
    default:
        throw std::invalid_argument(
            fmt::format("invalid rd_type: {}", rd_type_name(data_type)));
    }
}

static std::unique_ptr<char[], void (*)(void *)>
alloc_output_buffer(const rd::KW *rd_kw) {
    size_t sizeof_iotype = rd_kw->iotype_size();
    size_t buffer_size = rd_kw->size() * sizeof_iotype;
    auto buffer = rd::checked_calloc<char>(buffer_size);

    const auto &m_data = rd_kw->data();
    if (!m_data.has_value())
        /* RD_MESS_TYPE keywords carry no element-wise data. */
        return buffer;

    std::visit(
        [&](const auto &vec) {
            using VecT = std::decay_t<decltype(vec)>;
            if constexpr (std::is_same_v<VecT, std::vector<char>>) {
                /* RD_BOOL_TYPE, stored as vector<char> with 0/1 values. */
                for (size_t i = 0; i < vec.size(); i++) {
                    int value = vec[i] ? RD_BOOL_TRUE_INT : RD_BOOL_FALSE_INT;
                    std::memcpy(&buffer[i * sizeof_iotype], &value,
                                sizeof(value));
                }
                util_endian_flip_vector(buffer.get(), sizeof_iotype,
                                        vec.size());
            } else if constexpr (std::is_same_v<VecT,
                                                std::vector<std::string>>) {
                for (size_t i = 0; i < vec.size(); i++) {
                    size_t buffer_offset = i * sizeof_iotype;
                    size_t string_length =
                        std::min(vec[i].size(), sizeof_iotype);

                    std::memcpy(&buffer[buffer_offset], vec[i].data(),
                                string_length);

                    // Pad with spaces
                    for (size_t j = string_length; j < sizeof_iotype; j++)
                        buffer[buffer_offset + j] = ' ';
                }
            } else {
                /* RD_INT_TYPE / RD_FLOAT_TYPE / RD_DOUBLE_TYPE: element
                   layout matches the on-disk iotype directly. */
                if (!vec.empty())
                    std::memcpy(buffer.get(), vec.data(), buffer_size);
                util_endian_flip_vector(buffer.get(), sizeof_iotype,
                                        vec.size());
            }
        },
        m_data.value());

    return buffer;
}

bool rd::KW::size_and_type_equal(const rd::KW *rd_kw2) const {
    return (this->size() == rd_kw2->size() &&
            rd_type_is_equal(this->data_type(), rd_kw2->data_type()));
}

static bool rd_kw_header_eq(const rd::KW *rd_kw1, const rd::KW *rd_kw2) {
    return (fmt::format("{:8.8}", rd_kw1->name()) ==
            fmt::format("{:8.8}", rd_kw2->name())) &&
           rd_kw1->size_and_type_equal(rd_kw2);
}

/**
   This function compares two rd_kw instances, and returns true if they are equal.
*/

bool rd::KW::operator==(const rd::KW &other) const {
    if (!rd_kw_header_eq(this, &other))
        return false;
    auto &data1 = this->data();
    auto &data2 = other.data();
    if (!data1.has_value() || !data2.has_value())
        return !data1.has_value() && !data2.has_value();

    auto &vec1 = *data1;
    auto &vec2 = *data2;
    if (vec1.index() != vec2.index())
        return false;

    return std::visit(
        [&vec2](const auto &v1) -> bool {
            using T = std::decay_t<decltype(v1)>;
            const T &v2 = std::get<T>(vec2);
            if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                return v1 == v2;
            } else {
                // Apply byte-wise comparison for floating points to preserve NaN behavior
                if (v1.size() != v2.size())
                    return false;
                if (v1.empty())
                    return true;
                return std::memcmp(v1.data(), v2.data(),
                                   v1.size() *
                                       sizeof(typename T::value_type)) == 0;
            }
        },
        vec1);
}

template <typename T> bool approx_equal(T a, T b, T rel_diff, T abs_diff);
template <>
bool approx_equal<float>(float a, float b, float rel_diff, float abs_diff) {
    return util_float_approx_equal__(a, b, rel_diff, abs_diff);
}
template <>
bool approx_equal<double>(double a, double b, double rel_diff,
                          double abs_diff) {
    return util_double_approx_equal__(a, b, rel_diff, abs_diff);
}

bool rd::KW::approx_equal(const rd::KW &other, double abs_diff,
                          double rel_diff) const {
    if (!size_and_type_equal(&other))
        return false;

    if (!this->data().has_value() || !other.data().has_value())
        return this->m_data == other.m_data;

    return std::visit(
        [&](const auto &vec1) {
            using VecT = std::decay_t<decltype(vec1)>;
            const auto &vec2 = std::get<VecT>(other.m_data.value());

            if constexpr (std::is_same_v<VecT, std::vector<float>> ||
                          std::is_same_v<VecT, std::vector<double>>) {
                using T = typename VecT::value_type;
                if (vec1.size() != vec2.size())
                    return false;
                for (size_t index = 0; index < vec1.size(); index++) {
                    if (!::approx_equal<T>(vec1[index], vec2[index],
                                           static_cast<T>(rel_diff),
                                           static_cast<T>(abs_diff)))
                        return false;
                }
                return true;
            } else {
                return vec1 == vec2;
            }
        },
        this->data().value());
}

static size_t rd_kw_fortio_data_size(const rd::KW *rd_kw) {
    const size_t blocksize = get_blocksize(rd_kw->data_type());
    const size_t num_blocks =
        rd_kw->size() / blocksize + (rd_kw->size() % blocksize == 0 ? 0 : 1);

    return num_blocks * (4 + 4) +                // Fortran fluff for each block
           rd_kw->size() * rd_kw->iotype_size(); // Actual data
}

/**
   Returns the number of bytes this rd_kw instance would occupy in
   BINARY file; we add 2*4 to the header size to include the size of
   the fortran header and trailer combo.
*/

size_t rd::KW::fortio_size() const {
    size_t size = RD_KW_HEADER_FORTIO_SIZE;
    size += rd_kw_fortio_data_size(this);
    return size;
}

/** Create a new  copy of @other, where only the
   elements corresponding to the slice [index1:index2) is included.

   The input parameters @index1 and @index2 can to some extent be
   out-of-range:

       index1 = max( index1 , 0 );
       index2 = min( index2 , size );

   If index1 > index2 the result will be empty.
   Throws invalid_argument for stride == 0 and index1 >= other.size() */
rd::KW::KW(const rd::KW &other, size_t index1, size_t index2, size_t stride)
    : m_size(0), m_data_type(other.data_type()) {
    if (index2 > other.size())
        index2 = other.size();
    if (index1 >= other.size())
        throw std::invalid_argument(
            fmt::format("index1={} > size:{}", index1, other.size()));
    if (stride == 0)
        throw std::invalid_argument(
            fmt::format("stride:{} must be positive", stride));

    size_t src_index = index1;
    /* 1: Determine size of the sliced copy. */
    size_t new_size = 0;
    while (src_index < index2) {
        new_size++;
        src_index += stride;
    }

    this->m_name = other.name();
    this->m_size = new_size;

    if (other.m_data.has_value())
        this->m_data = std::visit(
            [index1, index2, stride](const auto &vec) -> rd::kw_data {
                using VecT = std::decay_t<decltype(vec)>;
                VecT result;
                for (size_t src_index = index1; src_index < index2;
                     src_index += stride)
                    result.push_back(vec[src_index]);
                return result;
            },
            other.m_data.value());
}

void rd::KW::resize(size_t new_size) {
    if (new_size != m_size) {
        m_size = new_size;
        if (m_data.has_value())
            std::visit([new_size](auto &vec) { vec.resize(new_size); },
                       m_data.value());
    }
}

/**
   Will allocate a copy of the src_kw. Will copy @count elements
   starting at @offset. If @count < 0 all remaining elements from
   @offset will be copied. If new_kw == NULL the new keyword will have
   the same header as the @src, otherwise the value @new_kw will be
   used.
*/

rd::KW::KW(const rd::KW &other, const std::optional<std::string> &new_kw,
           size_t offset, size_t count)
    : m_size(count), m_data_type(other.data_type()) {
    if (offset >= other.size())
        throw std::invalid_argument(
            fmt::format("invalid offset - limits: [{},{})", 0, other.size()));
    if ((count + offset) > other.size())
        throw std::invalid_argument(
            fmt::format("invalid count value: {}", count));

    if (new_kw.has_value())
        m_name = strip_name(*new_kw);
    else
        m_name = other.name();

    if (other.m_data.has_value())
        this->m_data = std::visit(
            [offset, count](const auto &vec) -> rd::kw_data {
                using VecT = std::decay_t<decltype(vec)>;
                return VecT(vec.begin() + offset, vec.begin() + offset + count);
            },
            other.m_data.value());
}

void rd::KW::zero_init_data() {
    switch (rd_type_get_type(data_type())) {
    case RD_INT_TYPE:
        m_data = std::vector<int>(size(), 0);
        break;
    case RD_FLOAT_TYPE:
        m_data = std::vector<float>(size(), 0.0f);
        break;
    case RD_DOUBLE_TYPE:
        m_data = std::vector<double>(size(), 0.0);
        break;
    case RD_BOOL_TYPE:
        m_data = std::vector<char>(size(), 0);
        break;
    case RD_CHAR_TYPE:
    case RD_STRING_TYPE:
        m_data = std::vector<std::string>(size());
        break;
    default:
        /* RD_MESS_TYPE carries no element-wise data and is not
           representable by rd::kw_data. */
        m_data = std::nullopt;
        break;
    }
}

static bool rd_kw_qskip(FILE *stream) {
    const char sep = '\'';
    const char space = ' ';
    const char newline = '\n';
    const char tab = '\t';
    bool OK = true;
    char c;
    bool cont = true;
    while (cont) {
        c = fgetc(stream);
        if (c == EOF) {
            cont = false;
            OK = false;
        } else {
            if (c == space || c == newline || c == tab)
                cont = true;
            else if (c == sep)
                cont = false;
        }
    }
    return OK;
}

static bool rd_kw_fscanf_qstring(char *s, const char *fmt, int len,
                                 FILE *stream) {
    const char null_char = '\0';
    char last_sep;
    bool OK;
    OK = rd_kw_qskip(stream);
    if (OK) {
        int read_count = 0;
        read_count += fscanf(stream, fmt, s);
        s[len] = null_char;
        read_count += fscanf(stream, "%c", &last_sep);

        if (read_count != 2)
            throw std::runtime_error(
                "reading 'xxxxxxxx' formatted string failed");
    }
    return OK;
}

/* This rather painful parsing is because formatted eclipse double
  format : 0.ddddD+01 - difficult to parse the 'D'.

*/
static double __fscanf_RD_double(FILE *stream, const char *fmt) {
    int read_count, power;
    double value, arg;
    read_count = fscanf(stream, fmt, &arg, &power);
    if (read_count == 2)
        value = arg * pow(10, power);
    else {
        throw std::runtime_error("read failed");
        value = -1;
    }
    return value;
}

static char read_formatted_bool(FILE *stream) {
    char bool_char = '\0';
    if (fscanf(stream, READ_FMT_BOOL, &bool_char) != 1)
        throw std::runtime_error("read failed - premature file end?");
    if (bool_char == BOOL_TRUE_CHAR)
        return 1;
    if (bool_char == BOOL_FALSE_CHAR)
        return 0;
    throw std::runtime_error(
        fmt::format("Logical value: [{}] not recogniced", bool_char));
}

static std::optional<rd::kw_data> read_formatted_data(rd::KW *rd_kw,
                                                      ERT::FortIO &fortio) {
    const rd_type_enum type = rd_kw->get_type();
    const size_t size = rd_kw->size();
    FILE *stream = fortio.get_FILE();
    const std::string read_format = read_fmt(rd_kw->data_type());
    std::optional<rd::kw_data> data;

    switch (type) {
    case RD_INT_TYPE: {
        std::vector<int> values(size);
        for (size_t i = 0; i < size; i++) {
            if (fscanf(stream, read_format.c_str(), &values[i]) != 1)
                throw std::runtime_error(fmt::format(
                    "after reading {} values reading of keyword:{} from:{} failed",
                    i, rd_kw->name(), fortio.filename_ref()));
        }
        data = std::move(values);
    } break;
    case RD_FLOAT_TYPE: {
        std::vector<float> values(size);
        for (size_t i = 0; i < size; i++) {
            if (fscanf(stream, read_format.c_str(), &values[i]) != 1)
                throw std::runtime_error(fmt::format(
                    "after reading {} values reading of keyword:{} from:{} failed",
                    i, rd_kw->name(), fortio.filename_ref()));
        }
        data = std::move(values);
    } break;
    case RD_DOUBLE_TYPE: {
        std::vector<double> values(size);
        for (size_t i = 0; i < size; i++)
            values[i] = __fscanf_RD_double(stream, read_format.c_str());
        data = std::move(values);
    } break;
    case RD_BOOL_TYPE: {
        std::vector<char> values(size);
        for (size_t i = 0; i < size; i++)
            values[i] = read_formatted_bool(stream);
        data = std::move(values);
    } break;
    case RD_CHAR_TYPE:
    case RD_STRING_TYPE: {
        const size_t width =
            type == RD_CHAR_TYPE ? RD_STRING8_LENGTH : rd_kw->iotype_size();
        std::vector<char> buf(width + 1, '\0');
        std::vector<std::string> values;
        values.reserve(size);
        for (size_t i = 0; i < size; i++) {
            rd_kw_fscanf_qstring(buf.data(), read_format.c_str(),
                                 static_cast<int>(width), stream);
            values.emplace_back(buf.data());
        }
        data = std::move(values);
    } break;
    case RD_MESS_TYPE: {
        char buf[RD_STRING8_LENGTH + 1];
        for (size_t i = 0; i < size; i++)
            rd_kw_fscanf_qstring(buf, read_format.c_str(), RD_STRING8_LENGTH,
                                 stream);
        /* leave data as nullopt. */
    } break;
    default:
        throw std::runtime_error(fmt::format(
            "Internal error: internal eclipse_type: {} not recognized", type));
    }

    /* Skip the trailing newline */
    fortio.fseek(1, SEEK_CUR);
    return data;
}

static bool read_unformatted_data(rd::KW *rd_kw, ERT::FortIO &fortio,
                                  std::optional<rd::kw_data> &out) {
    const rd_type_enum type = rd_kw->get_type();
    const size_t size = rd_kw->size();
    const size_t sizeof_iotype = rd_kw->iotype_size();
    if (sizeof_iotype != 0 &&
        size > std::numeric_limits<size_t>::max() / sizeof_iotype)
        throw std::invalid_argument(
            fmt::format("buffer size overflow: {} * {}", size, sizeof_iotype));

    const size_t record_size = size * sizeof_iotype;
    if (record_size > std::numeric_limits<int>::max())
        throw std::invalid_argument(
            "record size exceeded signed 32 bit integer");

    std::vector<char> buffer(record_size);
    bool read_ok =
        fortio.fread_buffer(buffer.data(), static_cast<int>(record_size));
    if (!read_ok)
        return false;

    if (RD_ENDIAN_FLIP) {
        if (rd_type_is_numeric(rd_kw->data_type()) ||
            rd_type_is_bool(rd_kw->data_type()))
            util_endian_flip_vector(buffer.data(), sizeof_iotype, size);
    }

    switch (type) {
    case RD_INT_TYPE: {
        std::vector<int> result(size);
        if (record_size > 0)
            std::memcpy(result.data(), buffer.data(), record_size);
        out = std::move(result);
    } break;
    case RD_FLOAT_TYPE: {
        std::vector<float> result(size);
        if (record_size > 0)
            std::memcpy(result.data(), buffer.data(), record_size);
        out = std::move(result);
    } break;
    case RD_DOUBLE_TYPE: {
        std::vector<double> result(size);
        if (record_size > 0)
            std::memcpy(result.data(), buffer.data(), record_size);
        out = std::move(result);
    } break;
    case RD_BOOL_TYPE: {
        std::vector<char> result(size);
        for (size_t i = 0; i < size; i++) {
            int int_value;
            std::memcpy(&int_value, &buffer[i * sizeof_iotype],
                        sizeof int_value);
            result[i] = (int_value == RD_BOOL_TRUE_INT) ? 1 : 0;
        }
        out = std::move(result);
    } break;
    case RD_CHAR_TYPE:
    case RD_STRING_TYPE: {
        std::vector<std::string> result;
        result.reserve(size);
        for (size_t i = 0; i < size; i++)
            result.emplace_back(&buffer[i * sizeof_iotype], sizeof_iotype);
        out = std::move(result);
    } break;
    default:
        /* RD_MESS_TYPE: leave out unset (nullopt). */
        out = std::nullopt;
        break;
    }
    return true;
}

bool rd::KW::fread_data(rd::KW *rd_kw, ERT::FortIO &fortio) {
    if (rd_kw->size() == 0) {
        /* The keyword has zero size - and reading data is trivially OK. */
        rd_kw->zero_init_data();
        return true;
    }

    if (fortio.fmt_file()) {
        rd_kw->m_data = read_formatted_data(rd_kw, fortio);
        return true;
    } else {
        std::optional<rd::kw_data> out;
        bool read_ok = read_unformatted_data(rd_kw, fortio, out);
        if (read_ok)
            rd_kw->m_data = std::move(out);
        return read_ok;
    }
}

/**
   Reads a selection of elements (given by @index_map) from the data
   section of a single keyword. The @kw_offset argument is the byte
   offset of the start of the keyword (i.e. its header) in the file,
   as stored by rd_file_kw.
*/
void rd::KW::fread_indexed_data(ERT::FortIO &fortio, offset_type kw_offset,
                                rd_data_type data_type, int element_count,
                                const std::vector<int> &index_map,
                                char *io_buffer) {
    size_t sizeof_iotype = rd_type_get_sizeof_iotype(data_type);

    // For unformatted (binary) files the individual elements have a fixed
    // on-disk size, so we can seek directly to each requested element. For
    // formatted (ASCII) files the elements have a variable text width and
    // direct seeking is not possible; in that case we read the whole
    // keyword via fread() and extract the requested elements from m_data
    // afterwards.
    if (fortio.fmt_file()) {
        fortio.fseek(kw_offset, SEEK_SET);
        std::unique_ptr<rd::KW> rd_kw = rd::KW::fread(fortio);
        if (rd_kw == NULL)
            throw std::runtime_error(fmt::format(
                "failed to load keyword at offset:{}", (long)kw_offset));

        if (!rd_kw->m_data.has_value())
            throw std::runtime_error(fmt::format(
                "failed to load keyword data at offset:{}", (long)kw_offset));

        std::visit(
            [&](const auto &vec) {
                using VecT = std::decay_t<decltype(vec)>;
                for (size_t index = 0; index < index_map.size(); index++) {
                    int element_index = index_map[index];
                    if (element_index < 0 || element_index >= element_count)
                        throw std::invalid_argument(fmt::format(
                            "Element index is out of range 0 <= {} < {}",
                            element_index, element_count));
                    char *dst = &io_buffer[index * sizeof_iotype];
                    if constexpr (std::is_same_v<VecT, std::vector<char>>) {
                        /* RD_BOOL_TYPE, stored as vector<char> with 0/1
                           values; on-disk representation is a full int. */
                        int int_value = vec[element_index] ? RD_BOOL_TRUE_INT
                                                           : RD_BOOL_FALSE_INT;
                        std::memcpy(dst, &int_value, sizeof_iotype);
                    } else if constexpr (std::is_same_v<
                                             VecT, std::vector<std::string>>) {
                        const std::string &s = vec[element_index];
                        size_t string_length =
                            std::min(s.size(), sizeof_iotype);
                        std::memcpy(dst, s.data(), string_length);
                        for (size_t j = string_length; j < sizeof_iotype; j++)
                            dst[j] = ' ';
                    } else {
                        std::memcpy(dst, &vec[element_index], sizeof_iotype);
                    }
                }
            },
            rd_kw->m_data.value());
    } else {
        const size_t block_size = get_blocksize(data_type);
        FILE *stream = fortio.get_FILE();
        offset_type data_offset = kw_offset + RD_KW_HEADER_FORTIO_SIZE;

        for (size_t index = 0; index < index_map.size(); index++) {
            int element_index = index_map[index];

            if (element_index < 0 || element_index >= element_count)
                throw std::invalid_argument(
                    fmt::format("Element index is out of range 0 <= {} < {}",
                                element_index, element_count));

            fortio.data_fseek(data_offset, element_index, sizeof_iotype,
                              element_count, block_size);
            util_fread(&io_buffer[index * sizeof_iotype], sizeof_iotype, 1,
                       stream, __func__);
        }

        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(io_buffer, sizeof_iotype, index_map.size());
    }
}

/**
   Allocates storage and reads data.
*/
bool rd::KW::fskip_data(rd_data_type data_type, const int element_count,
                        ERT::FortIO &fortio) {
    if (element_count <= 0)
        return true;

    bool fmt_file = fortio.fmt_file();
    if (fmt_file) {
        /* Formatted skipping actually involves reading the data - nice ??? */
        rd::KW tmp_kw{"WORK", element_count, data_type};
        fread_data(&tmp_kw, fortio);
    } else {
        size_t num_elements = static_cast<size_t>(element_count);
        const size_t blocksize = get_blocksize(data_type);
        const size_t block_count =
            num_elements / blocksize + (num_elements % blocksize != 0);
        size_t element_size = rd_type_get_sizeof_iotype(data_type);

        if (!fortio.data_fskip(element_size, num_elements, block_count))
            return false;
    }

    return true;
}

/**
   This function will skip the header part of an rd_kw instance. The
   function will read the file content at the current position, it is
   therefore essential that the file pointer is positioned at the
   beginning of a keyword when this function is called; otherwise it
   will be complete crash and burn.
*/

void rd::KW::fskip_header(ERT::FortIO &fortio) {
    bool fmt_file = fortio.fmt_file();
    if (fmt_file) {
        rd::KW::fread_header(fortio);
    } else
        fortio.fskip_record();
}

std::unique_ptr<rd::KW> rd::KW::fread_header(ERT::FortIO &fortio) {
    const char null_char = '\0';
    FILE *stream = fortio.get_FILE();
    bool fmt_file = fortio.fmt_file();
    char header[RD_STRING8_LENGTH + 1];
    char rd_type_str[RD_TYPE_LENGTH + 1];
    int size;

    if (fmt_file) {
        if (!rd_kw_fscanf_qstring(header, "%8c", 8, stream))
            return {nullptr};

        int read_count = fscanf(stream, "%d", &size);
        if (read_count != 1)
            return {nullptr};

        if (!rd_kw_fscanf_qstring(rd_type_str, "%4c", 4, stream))
            return {nullptr};

        fgetc(stream); /* Reading the trailing newline ... */
    } else {
        header[RD_STRING8_LENGTH] = null_char;
        rd_type_str[RD_TYPE_LENGTH] = null_char;
        int record_size = fortio.init_read();

        if (record_size <= 0)
            return {nullptr};

        char buffer[RD_KW_HEADER_DATA_SIZE];
        size_t read_bytes =
            std::fread(buffer, 1, RD_KW_HEADER_DATA_SIZE, stream);

        if (read_bytes != RD_KW_HEADER_DATA_SIZE)
            return {nullptr};

        memcpy(header, &buffer[0], RD_STRING8_LENGTH);
        void *ptr = &buffer[RD_STRING8_LENGTH];
        size = *((int *)ptr);

        memcpy(rd_type_str, &buffer[RD_STRING8_LENGTH + sizeof(size)],
               RD_TYPE_LENGTH);

        if (!fortio.complete_read(record_size))
            return {nullptr};

        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(&size, sizeof size, 1);
    }

    rd_data_type data_type = rd_type_create_from_name(rd_type_str);
    return std::make_unique<rd::KW>(header, size, data_type);
}

std::unique_ptr<rd::KW> rd::KW::fread(ERT::FortIO &fortio) {
    if (auto rd_kw = rd::KW::fread_header(fortio)) {
        if (!fread_data(rd_kw.get(), fortio))
            return {nullptr};

        return rd_kw;
    } else
        return {nullptr};
}

static void rd_kw_fwrite_data_unformatted(const rd::KW *rd_kw,
                                          ERT::FortIO &fortio) {
    auto iobuffer = alloc_output_buffer(rd_kw);
    size_t sizeof_iotype = rd_kw->iotype_size();
    {
        const size_t blocksize = get_blocksize(rd_kw->data_type());
        const size_t num_blocks = rd_kw->size() / blocksize +
                                  (rd_kw->size() % blocksize == 0 ? 0 : 1);
        for (size_t block_nr = 0; block_nr < num_blocks; block_nr++) {
            size_t blocksize_rem =
                std::min((block_nr + 1) * blocksize, rd_kw->size());
            size_t blocksize_prev = block_nr * blocksize;
            size_t this_blocksize = blocksize_prev > blocksize_rem
                                        ? 0
                                        : blocksize_rem - blocksize_prev;
            size_t record_size =
                this_blocksize *
                sizeof_iotype; /* The total size in bytes of the record written by the fortio layer. */
            if (record_size > std::numeric_limits<int>::max())
                throw std::invalid_argument(fmt::format(
                    "Size of record exceeded 32-bit signed integer"));
            fortio.fwrite_record(
                &iobuffer[block_nr * blocksize * sizeof_iotype],
                static_cast<int>(record_size));
        }
    }
}

static void rd_kw_fwrite_data_formatted(const rd::KW *rd_kw,
                                        ERT::FortIO &fortio) {
    const auto &m_data = rd_kw->data();
    if (!m_data.has_value()) {
        /* RD_MESS_TYPE keywords carry no element-wise data. */
        if (rd_kw->size() > 0)
            throw std::runtime_error("Internal inconsistency : message type "
                                     "keywords should not have data");
        return;
    }

    FILE *stream = fortio.get_FILE();
    const size_t blocksize = get_blocksize(rd_kw->data_type());
    const size_t columns = get_columns(rd_kw->data_type());
    const size_t string_width = rd_kw->iotype_size();
    const rd_type_enum type = rd_kw->get_type();
    const size_t num_blocks =
        rd_kw->size() / blocksize + (rd_kw->size() % blocksize == 0 ? 0 : 1);
    std::visit(
        [&](const auto &vec) {
            using VecT = std::decay_t<decltype(vec)>;
            for (size_t block_nr = 0; block_nr < num_blocks; block_nr++) {
                size_t block_next =
                    std::min((block_nr + 1) * blocksize, rd_kw->size());
                size_t block_prev = block_nr * blocksize;
                size_t this_blocksize =
                    block_prev > block_next ? 0 : block_next - block_prev;
                size_t num_lines = this_blocksize / columns +
                                   (this_blocksize % columns == 0 ? 0 : 1);
                for (size_t line_nr = 0; line_nr < num_lines; line_nr++) {
                    size_t num_columns =
                        std::min((line_nr + 1) * columns, this_blocksize) -
                        columns * line_nr;
                    for (size_t col_nr = 0; col_nr < num_columns; col_nr++) {
                        size_t data_index =
                            block_nr * blocksize + line_nr * columns + col_nr;
                        if (data_index >= vec.size())
                            throw std::logic_error(
                                "Loop exhausted size in "
                                "rd_kw_fwrite_data_formatted");
                        std::string element;
                        if constexpr (std::is_same_v<
                                          VecT, std::vector<std::string>>) {
                            if (type == RD_CHAR_TYPE)
                                element = rd::format_kw_element(
                                    vec[data_index].c_str());
                            else
                                element = rd::format_kw_element(
                                    vec[data_index].c_str(), string_width);
                        } else if constexpr (std::is_same_v<
                                                 VecT, std::vector<char>>) {
                            element =
                                rd::format_kw_element(vec[data_index] != 0);
                        } else {
                            element = rd::format_kw_element(vec[data_index]);
                        }
                        fputs(element.c_str(), stream);
                    }
                    fprintf(stream, "\n");
                }
            }
        },
        m_data.value());
}

void rd::KW::fwrite_data(ERT::FortIO &fortio) const {
    bool fmt_file = fortio.fmt_file();

    if (fmt_file)
        rd_kw_fwrite_data_formatted(this, fortio);
    else
        rd_kw_fwrite_data_unformatted(this, fortio);
}

void rd_kw_fwrite_header(const rd::KW *rd_kw, ERT::FortIO &fortio) {
    FILE *stream = fortio.get_FILE();
    bool fmt_file = fortio.fmt_file();
    std::string type_name = rd_type_name(rd_kw->data_type());

    if (rd_kw->size() > std::numeric_limits<int>::max())
        throw std::invalid_argument(
            fmt::format("Size of rd_kw exceeds format: {}", rd_kw->size()));

    std::string header8 = fmt::format("{:8.8}", rd_kw->name());

    if (fmt_file)
        fprintf(stream, WRITE_HEADER_FMT, header8.c_str(),
                static_cast<int>(rd_kw->size()), type_name.c_str());
    else {
        int size = static_cast<int>(rd_kw->size());
        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(&size, sizeof size, 1);

        fortio.init_write(RD_KW_HEADER_DATA_SIZE);

        std::fwrite(header8.c_str(), sizeof(char), RD_STRING8_LENGTH, stream);
        std::fwrite(&size, sizeof(int), 1, stream);
        std::fwrite(type_name.c_str(), sizeof(char), RD_TYPE_LENGTH, stream);

        fortio.complete_write(RD_KW_HEADER_DATA_SIZE);
    }
}

bool rd::KW::fwrite(ERT::FortIO &fortio) const {
    if (this->name().size() > RD_STRING8_LENGTH) {
        fortio.fwrite_error();
        return false;
    }
    rd_kw_fwrite_header(this, fortio);
    this->fwrite_data(fortio);
    return true;
}

int rd::kw_get_size(const rd::KW *rd_kw) {
    if (rd_kw->size() > std::numeric_limits<int>::max())
        throw std::invalid_argument(
            fmt::format("Size of rd_kw exceeded int max: {}", rd_kw->size()));
    return static_cast<int>(rd_kw->size());
}

std::unique_ptr<rd::KW> rd::KW::global_copy(const rd::KW *src,
                                            const rd::KW *actnum) {
    if (actnum->get_type() != RD_INT_TYPE)
        return NULL;

    const size_t global_size = actnum->size();
    auto global_copy =
        std::make_unique<rd::KW>(src->name(), global_size, src->data_type());
    auto &mapping = actnum->get_vector<int>();
    const size_t src_size = src->size();
    size_t src_index = 0;
    bool overflow = false;
    std::visit(
        [&](auto &&target_alt) {
            using T = typename std::decay_t<decltype(target_alt)>::value_type;
            auto &target_vec = global_copy->get_vector<T>();
            const auto &src_vec = src->get_vector<T>();
            for (size_t global_index = 0; global_index < global_size;
                 global_index++) {
                if (mapping[global_index]) {
                    /* We ran through and beyond the size of the src keyword. */
                    if (src_index >= src_size) {
                        overflow = true;
                        break;
                    }
                    target_vec[global_index] = src_vec[src_index];
                    src_index++;
                }
            }
        },
        global_copy->data().value());

    if (overflow)
        global_copy.reset(nullptr);

    /* Not all the src data was distributed. */
    if (src_index < src_size) {
        global_copy.reset(nullptr);
    }

    return global_copy;
}

bool rd::KW::size_and_numeric_type_equal(const rd::KW *kw2) const {
    return this->size_and_type_equal(kw2) &&
           rd_type_is_numeric(this->data_type());
}

void rd::KW::operator-=(const rd::KW &sub_kw) {
    std::visit(
        [&](auto &&target_alt) {
            using T = typename std::decay_t<decltype(target_alt)>::value_type;
            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> ||
                          std::is_same_v<T, double>) {
                if (!this->size_and_numeric_type_equal(&sub_kw))
                    throw std::invalid_argument("type/size  mismatch");
                auto &target_data = this->get_vector<T>();
                const auto &sub_data = sub_kw.get_vector<T>();
                for (size_t i = 0; i < target_data.size(); i++)
                    target_data[i] -= sub_data[i];
            } else
                throw std::invalid_argument(
                    fmt::format("inplace sub not implemented for type:{}",
                                rd_type_name(this->data_type())));
        },
        this->data().value());
}

static bool elm_equal_numeric(const rd::KW *rd_kw1, const rd::KW *rd_kw2,
                              size_t offset, double abs_epsilon,
                              double rel_epsilon) {
    double v1 = rd_kw1->as_double(offset);
    double v2 = rd_kw2->as_double(offset);
    return util_double_approx_equal__(v1, v2, rel_epsilon, abs_epsilon);
}

size_t rd::KW::first_different(const rd::KW *rd_kw2, size_t offset,
                               double abs_epsilon, double rel_epsilon) const {
    if (!size_and_type_equal(rd_kw2))
        throw std::invalid_argument("sorry invalid comparison");

    if (offset >= this->size())
        throw std::invalid_argument(fmt::format(
            "offset value in first_difference exceeded size: {}", offset));

    bool numeric_compare =
        ((abs_epsilon > 0) || (rel_epsilon > 0)) &&
        ((get_type() == RD_FLOAT_TYPE) || (get_type() == RD_DOUBLE_TYPE));

    if (numeric_compare) {
        for (size_t index = offset; index < this->size(); index++)
            if (!elm_equal_numeric(this, rd_kw2, index, abs_epsilon,
                                   rel_epsilon))
                return index;
        return this->size();
    } else {
        auto &vec1 = this->data().value();
        auto &vec2 = rd_kw2->data().value();
        return std::visit(
            [&vec2, offset](const auto &v1) -> size_t {
                using T = std::decay_t<decltype(v1)>;
                const T &v2 = std::get<T>(vec2);
                if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                    for (size_t index = offset; index < v2.size(); index++)
                        if (v1[index] != v2[index])
                            return index;
                    return v2.size();
                } else {
                    // Apply byte-wise comparison for floating points to preserve NaN behavior
                    using ElemT = typename T::value_type;
                    auto it1 =
                        std::mismatch(
                            v1.begin() + offset, v1.begin() + v1.size(),
                            v2.begin() + offset,
                            [](const ElemT &a, const ElemT &b) {
                                return std::memcmp(&a, &b, sizeof(ElemT)) == 0;
                            })
                            .first;
                    if (it1 == v1.begin() + v1.size()) {
                        return v1.size(); // No mismatch found
                    }
                    return std::distance(v1.begin(), it1);
                }
            },
            vec1);
    }
}

/*
  This is an extremely special-case function written for the region
  creation code. Given a completed rd_kw regions keyword, the purpose
  of this function is to "detect and correct" uninitialized cells with
  value 0. This function is purely heuristic:

   1. It only considers cells which are active in the grid, i.e. where
      actnum[] != 0.

   2. It will scan the four neighbours in the xy plane, if all
      neighbours agree on region value this value will be applied;
      otherwise the value will not be changed. Neighbouring cells with
      value zero are not considered when comparing.
*/
void rd::KW::fix_uninitialized(int nx, int ny, int nz, const int *actnum) {
    int i, j, k;
    std::vector<int> &data = get_vector<int>();

    auto undetermined1 = std::make_unique<std::vector<int>>();
    auto undetermined2 = std::make_unique<std::vector<int>>();

    for (k = 0; k < nz; k++) {
        undetermined1->clear();
        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                int g0 = i + j * nx + k * nx * ny;

                if (data[g0] == 0 && actnum[g0])
                    undetermined1->push_back(g0);
            }
        }

        while (true) {
            bool finished = true;

            undetermined2->clear();
            for (auto g0 : *undetermined1) {
                int j = (g0 - k * nx * ny) / nx;
                int i = g0 - k * nx * ny - j * nx;

                if (data[g0] == 0 && actnum[g0]) {
                    int n1 = 0;
                    int n2 = 0;
                    int n3 = 0;
                    int n4 = 0;

                    if (i > 0) {
                        int g1 = g0 - 1;
                        if (actnum[g1])
                            n1 = data[g1];
                    }

                    if (i < (nx - 1)) {
                        int g2 = g0 + 1;
                        if (actnum[g2])
                            n2 = data[g2];
                    }

                    if (j > 0) {
                        int g3 = g0 - nx;
                        if (actnum[g3])
                            n3 = data[g3];
                    }

                    if (j < (ny - 1)) {
                        int g4 = g0 + nx;
                        if (actnum[g4])
                            n4 = data[g4];
                    }

                    {
                        int new_value = 0;

                        if (n1)
                            new_value = n1;

                        if (n2) {
                            if (new_value == 0)
                                new_value = n2;
                            else if (new_value != n2)
                                new_value = -1;
                        }

                        if (n3) {
                            if (new_value == 0)
                                new_value = n3;
                            else if (new_value != n3)
                                new_value = -1;
                        }

                        if (n4) {
                            if (new_value == 0)
                                new_value = n4;
                            else if (new_value != n4)
                                new_value = -1;
                        }

                        if (new_value > 0) {
                            data[g0] = new_value;
                            finished = false;
                        }
                    }
                    if ((n1 + n2 + n3 + n4) == 0)
                        undetermined2->push_back(g0);
                }
            }
            undetermined1.swap(undetermined2);
            if (finished || undetermined1->empty())
                break;
        }
    }
}

std::unique_ptr<rd::KW> rd::KW::make_actnum(const rd::KW *porv_kw,
                                            float porv_limit) {
    if (!rd_type_is_float(porv_kw->data_type()))
        return NULL;

    if (porv_kw->name() != PORV_KW)
        return NULL;

    const size_t size = porv_kw->size();
    std::vector<int> actnum_values(size, 0);
    const std::vector<float> &porv_values = porv_kw->get_vector<float>();

    for (size_t i = 0; i < size; i++) {
        if (porv_values[i] > porv_limit)
            actnum_values[i] = 1;
        else
            actnum_values[i] = 0;
    }

    return std::make_unique<rd::KW>(ACTNUM_KW, actnum_values);
}
