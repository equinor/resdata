#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <fmt/format.h>

#include <resdata/rd_kw_header.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_util.hpp>

#include "ert/util/util.hpp"

std::string rd::KWHeader::strip_name(const std::string &name) {
    if (name.size() > RD_STRING8_LENGTH)
        return name;
    const size_t start = name.find_first_not_of(' ');
    if (start == std::string::npos)
        return std::string();
    const size_t end = name.find_last_not_of(' ');
    return name.substr(start, end - start + 1);
}

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

static bool skip_space_until_quote(std::istream &stream) {
    const char sep = '\'';
    const char space = ' ';
    const char newline = '\n';
    const char tab = '\t';
    bool OK = true;
    int c;
    bool cont = true;
    while (cont) {
        c = stream.get();
        if (c == EOF) {
            cont = false;
            OK = false;
        } else {
            char cc = static_cast<char>(c);
            if (cc == space || cc == newline || cc == tab)
                cont = true;
            else if (cc == sep)
                cont = false;
        }
    }
    return OK;
}

/** Reads a string separated by ' but assumed to be of size len */
static bool read_sized_quoted_string(char *s, size_t len,
                                     std::istream &stream) {
    bool OK = skip_space_until_quote(stream);
    if (OK) {
        stream.read(s, len);
        s[len] = '\0';
        char last_sep = '\0';
        stream.get(last_sep);

        if (stream.fail() || last_sep != '\'')
            throw std::runtime_error(
                "reading 'xxxxxxxx' formatted string failed");
    }
    return OK;
}

/* This rather painful parsing is because formatted eclipse double
  format : 0.ddddD+01 - difficult to parse the 'D'.

  Note: this deliberately does *not* use stream >> arg to extract the
  mantissa directly. Some standard library implementations (notably
  libc++ on macOS) collect hexfloat-alphabet characters ('a'-'f'/'A'-'F')
  while scanning a floating point literal - even without a "0x" prefix -
  before handing the buffer to strtod. That causes the 'D' exponent
  marker to be silently consumed as part of the mantissa scan, which
  then makes the exponent unreadable. Instead we read the whole
  whitespace-delimited token as a plain string first (which has no such
  floating-point-specific lookahead), then split it on 'D' ourselves and
  parse the two halves with strtod/strtol. */
static double parse_double(std::istream &stream) {
    std::string token;
    stream >> token;
    if (!stream)
        throw std::runtime_error("read failed");

    const size_t dpos = token.find('D');
    if (dpos == std::string::npos || dpos == 0)
        throw std::runtime_error("read failed");

    const std::string mantissa = token.substr(0, dpos);
    const std::string exponent = token.substr(dpos + 1);

    char *end = nullptr;
    double arg = std::strtod(mantissa.c_str(), &end);
    if (end != mantissa.c_str() + mantissa.size())
        throw std::runtime_error("read failed");

    end = nullptr;
    long power = std::strtol(exponent.c_str(), &end, 10);
    if (exponent.empty() || end != exponent.c_str() + exponent.size())
        throw std::runtime_error("read failed");

    return arg * pow(10, power);
}

/* Reads one formatted numeric value with stream::operator>>, throwing with a
 * helpful message (including how far we got) on failure. */
template <typename T>
static void read_formatted_value(std::istream &stream, T &value, size_t index,
                                 const std::string name, ERT::FortIO &fortio) {
    stream >> value;
    if (stream.fail())
        throw std::runtime_error(
            fmt::format("after reading {} values reading of keyword:{:8.8} "
                        "from:{} failed",
                        index, name, fortio.filename_ref()));
}

static char read_formatted_bool(std::istream &stream) {
    stream >> std::ws;
    char bool_char = '\0';
    stream.get(bool_char);
    if (stream.fail())
        throw std::runtime_error("read failed - premature file end?");
    if (bool_char == BOOL_TRUE_CHAR)
        return 1;
    if (bool_char == BOOL_FALSE_CHAR)
        return 0;
    throw std::runtime_error(
        fmt::format("Logical value: [{}] not recogniced", bool_char));
}

std::optional<rd::kw_data>
rd::KWHeader::read_formatted_data(ERT::FortIO &fortio) {
    std::istream &stream = fortio.get_istream();
    std::optional<rd::kw_data> data;

    switch (data_type().type) {
    case RD_INT_TYPE: {
        std::vector<int> values(size());
        for (size_t i = 0; i < size(); i++)
            read_formatted_value(stream, values[i], i, name(), fortio);
        data = std::move(values);
    } break;
    case RD_FLOAT_TYPE: {
        std::vector<float> values(size());
        for (size_t i = 0; i < size(); i++)
            read_formatted_value(stream, values[i], i, name(), fortio);
        data = std::move(values);
    } break;
    case RD_DOUBLE_TYPE: {
        std::vector<double> values(size());
        for (size_t i = 0; i < size(); i++)
            values[i] = parse_double(stream);
        data = std::move(values);
    } break;
    case RD_BOOL_TYPE: {
        std::vector<char> values(size());
        for (size_t i = 0; i < size(); i++)
            values[i] = read_formatted_bool(stream);
        data = std::move(values);
    } break;
    case RD_CHAR_TYPE:
    case RD_STRING_TYPE: {

        size_t iotype_size = rd_type_get_sizeof_iotype(data_type());
        const size_t width =
            data_type().type == RD_CHAR_TYPE ? RD_STRING8_LENGTH : iotype_size;
        std::vector<char> buf(width + 1);
        std::vector<std::string> values;
        values.reserve(size());
        for (size_t i = 0; i < size(); i++) {
            read_sized_quoted_string(buf.data(), width, stream);
            values.emplace_back(buf.data());
        }
        data = std::move(values);
    } break;
    case RD_MESS_TYPE: {
        char buf[RD_STRING8_LENGTH + 1];
        for (size_t i = 0; i < size(); i++)
            read_sized_quoted_string(buf, RD_STRING8_LENGTH, stream);
        /* leave data as nullopt. */
    } break;
    default:
        throw std::runtime_error(fmt::format(
            "Internal error: internal eclipse_type: {} not recognized",
            data_type().type));
    }

    /* Skip the trailing newline */
    fortio.fseek(1, SEEK_CUR);
    return data;
}

std::optional<rd::kw_data>
rd::KWHeader::read_unformatted_data(ERT::FortIO &fortio) {
    std::optional<rd::kw_data> out;
    const size_t sizeof_iotype = rd_type_get_sizeof_iotype(data_type());
    if (sizeof_iotype != 0 &&
        size() > std::numeric_limits<size_t>::max() / sizeof_iotype)
        throw std::invalid_argument(fmt::format("buffer size overflow: {} * {}",
                                                size(), sizeof_iotype));

    const size_t record_size = size() * sizeof_iotype;
    if (record_size > std::numeric_limits<int>::max())
        throw std::invalid_argument(
            "record size exceeded signed 32 bit integer");

    std::vector<char> buffer(record_size);
    bool read_ok =
        fortio.fread_buffer(buffer.data(), static_cast<int>(record_size));
    if (!read_ok)
        throw std::runtime_error("Could not read from buffer");

    if (RD_ENDIAN_FLIP) {
        if (rd_type_is_numeric(data_type()) || rd_type_is_bool(data_type()))
            util_endian_flip_vector(buffer.data(), sizeof_iotype, size());
    }

    switch (data_type().type) {
    case RD_INT_TYPE: {
        std::vector<int> result(size());
        if (record_size > 0)
            std::memcpy(result.data(), buffer.data(), record_size);
        out = std::move(result);
    } break;
    case RD_FLOAT_TYPE: {
        std::vector<float> result(size());
        if (record_size > 0)
            std::memcpy(result.data(), buffer.data(), record_size);
        out = std::move(result);
    } break;
    case RD_DOUBLE_TYPE: {
        std::vector<double> result(size());
        if (record_size > 0)
            std::memcpy(result.data(), buffer.data(), record_size);
        out = std::move(result);
    } break;
    case RD_BOOL_TYPE: {
        std::vector<char> result(size());
        for (size_t i = 0; i < size(); i++) {
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
        result.reserve(size());
        for (size_t i = 0; i < size(); i++)
            result.emplace_back(&buffer[i * sizeof_iotype], sizeof_iotype);
        out = std::move(result);
    } break;
    default:
        /* RD_MESS_TYPE: leave out unset (nullopt). */
        out = std::nullopt;
        break;
    }
    return out;
}

std::optional<rd::kw_data> rd::KWHeader::zero_init_data() {
    switch (rd_type_get_type(data_type())) {
    case RD_INT_TYPE:
        return std::vector<int>(size(), 0);
    case RD_FLOAT_TYPE:
        return std::vector<float>(size(), 0.0f);
    case RD_DOUBLE_TYPE:
        return std::vector<double>(size(), 0.0);
    case RD_BOOL_TYPE:
        return std::vector<char>(size(), 0);
    case RD_CHAR_TYPE:
    case RD_STRING_TYPE:
        return std::vector<std::string>(size());
        break;
    default:
        /* RD_MESS_TYPE carries no element-wise data and is not
           representable by rd::kw_data. */
        return std::nullopt;
    }
}

std::optional<rd::kw_data> rd::KWHeader::fread_data(ERT::FortIO &fortio) {
    if (size() == 0)
        return zero_init_data();
    else if (fortio.fmt_file())
        return read_formatted_data(fortio);
    else
        return read_unformatted_data(fortio);
}

rd::KWHeader rd::KWHeader::fread(ERT::FortIO &fortio) {
    const char null_char = '\0';
    std::istream &stream = fortio.get_istream();
    char name[RD_STRING8_LENGTH + 1];
    char rd_type_str[RD_TYPE_LENGTH + 1];
    int size;

    if (fortio.fmt_file()) {
        if (!read_sized_quoted_string(name, 8, stream))
            throw std::runtime_error("Could not read name for keyword");

        stream >> size;
        if (stream.fail())
            throw std::runtime_error("Could not read size for keyword");

        if (!read_sized_quoted_string(rd_type_str, 4, stream))
            throw std::runtime_error("Could not read type for keyword");

        stream.get(); /* Reading the trailing newline ... */
    } else {
        name[RD_STRING8_LENGTH] = null_char;
        rd_type_str[RD_TYPE_LENGTH] = null_char;
        int record_size = fortio.init_read();

        if (record_size <= 0)
            throw std::runtime_error(
                "Record had zero size in reading keyword header");

        char buffer[RD_KW_HEADER_DATA_SIZE];
        if (!stream.read(buffer, RD_KW_HEADER_DATA_SIZE))
            throw std::runtime_error("Could not read name in keyword header");

        memcpy(name, &buffer[0], RD_STRING8_LENGTH);
        void *ptr = &buffer[RD_STRING8_LENGTH];
        size = *((int *)ptr);

        memcpy(rd_type_str, &buffer[RD_STRING8_LENGTH + sizeof(size)],
               RD_TYPE_LENGTH);

        if (!fortio.complete_read(record_size))
            throw std::runtime_error(
                "End record did not match in reading keyword header");

        if (RD_ENDIAN_FLIP)
            util_endian_flip_vector(&size, sizeof size, 1);
    }

    rd_data_type data_type = rd_type_create_from_name(rd_type_str);
    if (size < 0)
        throw std::runtime_error("Keyword header had negative size");
    return {static_cast<size_t>(size), data_type, name};
}
