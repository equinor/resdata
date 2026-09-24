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

namespace rd {
class KWHeader {
private:
    size_t m_size;
    rd_data_type m_data_type;
    std::string m_name;

public:
    KWHeader(size_t size, rd_data_type data_type, std::string name)
        : m_size(size), m_data_type(data_type), m_name(std::move(name)) {}
    size_t size() const { return m_size; }
    rd_data_type data_type() const { return m_data_type; }
    std::string name() const { return m_name; }
    void set_name(std::string name) { m_name = name; }
    void set_size(size_t size) { m_size = size; }
};

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

std::optional<kw_data> fread_data(const rd_data_type type, const size_t size,
                                  const std::string &name, ERT::FortIO &fortio);
std::optional<kw_data> zero_init_data(rd_data_type data_type, size_t size);
bool read_sized_quoted_string(char *s, size_t len, std::istream &stream);
} // namespace rd
