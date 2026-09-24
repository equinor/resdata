#pragma once
#include <cstddef>
#include <istream>
#include <utility>
#include <variant>
#include <vector>
#include <string>
#include <optional>

#include <resdata/rd_type.hpp>
#include <resdata/FortIO.hpp>

/* Character data in restart format files comes as an array of fixed-length
   string. Each of these strings is 8 characters long. The type name,
   i.e. 'REAL', 'INTE', ... , come as 4 character strings. */
#define RD_KW_HEADER_DATA_SIZE RD_STRING8_LENGTH + RD_TYPE_LENGTH + 4
#define RD_KW_HEADER_FORTIO_SIZE RD_KW_HEADER_DATA_SIZE + 8

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

/* For some peculiar reason the keyword data is written in blocks, all
   numeric data is in blocks of 1000 elements, and character data is
   in blocks of 105 elements. */
constexpr size_t BLOCKSIZE_NUMERIC = 1000;
constexpr size_t BLOCKSIZE_CHAR = 105;

inline size_t get_blocksize(rd_data_type data_type) {
    if (rd_type_is_alpha(data_type))
        return BLOCKSIZE_CHAR;

    return BLOCKSIZE_NUMERIC;
}

class KWHeader {
private:
    size_t m_size;
    rd_data_type m_data_type;
    std::string m_name;

    static std::string strip_name(const std::string &name);
    std::optional<rd::kw_data> read_formatted_data(ERT::FortIO &fortio);
    bool skip_formatted_data(ERT::FortIO &fortio);
    std::optional<rd::kw_data> read_unformatted_data(ERT::FortIO &fortio);

public:
    KWHeader(size_t size, rd_data_type data_type, const std::string &name)
        : m_size(size), m_data_type(data_type), m_name(strip_name(name)) {}
    size_t size() const { return m_size; }
    rd_data_type data_type() const { return m_data_type; }
    std::string name() const { return m_name; }
    void set_name(std::string name) { m_name = strip_name(name); }
    void set_size(size_t size) { m_size = size; }
    static KWHeader fread(ERT::FortIO &fortio);
    static void fskip(ERT::FortIO &fortio);
    bool fskip_data(ERT::FortIO &fortio);
    std::optional<kw_data> fread_data(ERT::FortIO &fortio);
    std::optional<kw_data> zero_init_data();
};
} // namespace rd
