#pragma once
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cmath>

#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <fmt/format.h>

#include <ert/util/util.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>

typedef struct rd_kw_struct rd_kw_type;
using rd_kw_ptr = std::unique_ptr<rd_kw_type>;

void rd_kw_set_header_name(rd_kw_type *, const char *);
void rd_kw_memcpy(rd_kw_type *, const rd_kw_type *);
void rd_kw_set_memcpy_data(rd_kw_type *, const void *);

/* the rd_kw datastructure is tightly bound to the on-disk binary format
   supplied by Eclipse, and there the number of elements is stored as a signed
   32 bit integer. Internally, size_t is used to denote size, however when
   loaded or saved to disk, the size is validated to be no larger than the
   std::numeric_limits<int>::max */

struct rd_kw_struct {
private:
    void init_data() {
        this->data = (char *)calloc(size, rd_type_get_sizeof_ctype(data_type));
        if (this->data == nullptr) {
            std::free(this->data);
            throw std::bad_alloc{};
        }
    }

public:
    size_t size;
    rd_data_type data_type;
    char *header8 =
        nullptr; /* Header which is right padded with ' ' to become exactly 8 characters long. Should only be used internally.*/
    char *header = nullptr;   /* Header which is trimmed to no-space. */
    char *data = nullptr;     /* The actual data vector. */
    bool shared_data = false; /* Whether this keyword has shared data or not. */

    struct shared_ref {
        void *data;
    };

    template <typename T> T at(size_t index) const;
    double as_double(size_t index) const;

    rd_kw_struct(rd_data_type data_type) = delete;

    rd_kw_struct(const char *header, size_t size, rd_data_type data_type,
                 const void *data = nullptr)
        : size(size), data_type(data_type) {
        rd_kw_set_header_name(this, header);
        init_data();
        rd_kw_set_memcpy_data(this, data);
    }

    rd_kw_struct(const char *header, int size, rd_data_type data_type,
                 const void *data = nullptr)
        : data_type(data_type) {
        if (size < 0)
            throw std::invalid_argument(
                fmt::format("rd_kw size was negative: {}", size));
        this->size = static_cast<size_t>(size);
        rd_kw_set_header_name(this, header);
        init_data();
        rd_kw_set_memcpy_data(this, data);
    }

    /* Non-owning constructor */
    rd_kw_struct(const char *header, int size, rd_data_type data_type,
                 shared_ref ref)
        : data_type(data_type) {
        if (size < 0)
            throw std::invalid_argument(
                fmt::format("rd_kw size was negative: {}", size));
        this->size = static_cast<size_t>(size);
        rd_kw_set_header_name(this, header);
        this->data = (char *)ref.data;
        this->shared_data = true;
    }

    rd_kw_struct(const rd_kw_struct &other)
        : size(other.size), data_type(other.data_type) {
        init_data();
        rd_kw_memcpy(this, &other);
    }
    rd_kw_struct(const rd_kw_struct &other, const char *new_kw, size_t offset,
                 size_t count);

    rd_kw_struct(const rd_kw_type &other, size_t index1, size_t index2,
                 int stride);

    ~rd_kw_struct() {
        std::free(header);
        std::free(header8);
        if (!shared_data)
            std::free(data);
    }
    static rd_kw_ptr fread(ERT::FortIO &fortio);
    static rd_kw_ptr make_actnum(const rd_kw_type *porv_kw, float porv_limit);
    static rd_kw_ptr global_copy(const rd_kw_type *src,
                                 const rd_kw_type *actnum);
};

/*
  Character data in restart format files comes as an array of fixed-length
  string. Each of these strings is 8 characters long. The type name,
  i.e. 'REAL', 'INTE', ... , come as 4 character strings.
*/
#define RD_KW_HEADER_DATA_SIZE RD_STRING8_LENGTH + RD_TYPE_LENGTH + 4
#define RD_KW_HEADER_FORTIO_SIZE RD_KW_HEADER_DATA_SIZE + 8

size_t rd_kw_first_different(const rd_kw_type *kw1, const rd_kw_type *kw2,
                             size_t offset, double abs_epsilon,
                             double rel_epsilon);
size_t rd_kw_fortio_size(const rd_kw_type *rd_kw);
void *rd_kw_get_ptr(const rd_kw_type *rd_kw);
void rd_kw_set_data_ptr(rd_kw_type *rd_kw, void *data);
void rd_kw_fwrite_data(const rd_kw_type *_rd_kw, ERT::FortIO &fortio);

namespace rd {
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
} // namespace rd

rd_data_type rd_kw_get_data_type(const rd_kw_type *);
const char *rd_kw_get_header(const rd_kw_type *rd_kw);
rd_kw_ptr rd_kw_fread_header(ERT::FortIO &);
bool rd_kw_fseek_kw(const char *, bool, bool, ERT::FortIO &);
void rd_kw_fskip(ERT::FortIO &);
void rd_kw_fread_indexed_data(ERT::FortIO &fortio, offset_type kw_offset,
                              rd_data_type, int element_count,
                              const std::vector<int> &index_map, char *buffer);
void rd_kw_resize(rd_kw_type *rd_kw, size_t new_size);
void rd_kw_memcpy(rd_kw_type *, const rd_kw_type *);
void rd_kw_get_memcpy_data(const rd_kw_type *, void *);
bool rd_kw_fwrite(const rd_kw_type *, ERT::FortIO &);
void rd_kw_iget(const rd_kw_type *, int, void *);
void rd_kw_iset(rd_kw_type *rd_kw, int i, const void *iptr);
void rd_kw_iset_char_ptr(rd_kw_type *rd_kw, size_t index, const char *s);
void rd_kw_iset_string8(rd_kw_type *rd_kw, size_t index, const char *s8);
void rd_kw_iset_string_ptr(rd_kw_type *, int, const char *);
const char *rd_kw_iget_string_ptr(const rd_kw_type *, int);
const char *rd_kw_iget_char_ptr(const rd_kw_type *rd_kw, int i);
void *rd_kw_iget_ptr(const rd_kw_type *, int);
int rd_kw_get_size(const rd_kw_type *);
size_t rd_kw_size(const rd_kw_type *);
double rd_kw_iget_as_double(const rd_kw_type *rd_kw, int i);
bool rd_kw_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2);
bool rd_kw_size_and_type_equal(const rd_kw_type *rd_kw1,
                               const rd_kw_type *rd_kw2);
bool rd_kw_icmp_string(const rd_kw_type *rd_kw, int index,
                       const char *other_string);
bool rd_kw_numeric_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2,
                         double abs_diff, double rel_diff);
bool rd_kw_data_equal(const rd_kw_type *rd_kw, const void *data);
bool rd_kw_content_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2);
bool rd_kw_fskip_data__(rd_data_type, int, ERT::FortIO &);
bool rd_kw_fskip_data(rd_kw_type *rd_kw, ERT::FortIO &fortio);
void rd_kw_fskip_header(ERT::FortIO &fortio);
bool rd_kw_size_and_numeric_type_equal(const rd_kw_type *kw1,
                                       const rd_kw_type *kw2);
bool rd_kw_inplace_safe_div(rd_kw_type *target_kw, const rd_kw_type *divisor);
void rd_kw_inplace_sqrt(rd_kw_type *kw);

int rd_kw_element_sum_int(const rd_kw_type *rd_kw);
double rd_kw_element_sum_float(const rd_kw_type *rd_kw);
void rd_kw_element_sum(const rd_kw_type *, void *);
void rd_kw_element_sum_indexed(const rd_kw_type *rd_kw,
                               const std::vector<int> &index_list, void *_sum);
void rd_kw_max_min(const rd_kw_type *, void *, void *);
void *rd_kw_get_void_ptr(const rd_kw_type *rd_kw);

void rd_kw_memcpy_data(rd_kw_type *target, const rd_kw_type *src);

void rd_kw_scalar_set_float_or_double(rd_kw_type *rd_kw, double value);

#define RD_KW_SCALAR_SET_TYPED_HEADER(ctype)                                   \
    void rd_kw_scalar_set_##ctype(rd_kw_type *rd_kw, ctype value);
RD_KW_SCALAR_SET_TYPED_HEADER(int)
RD_KW_SCALAR_SET_TYPED_HEADER(float)
RD_KW_SCALAR_SET_TYPED_HEADER(double)
#undef RD_KW_SCALAR_SET_TYPED_HEADER

rd_kw_type *rd_kw_alloc_scatter_copy(const rd_kw_type *src_kw, int target_size,
                                     const int *mapping, void *def_value);

void rd_kw_inplace_add_squared(rd_kw_type *target_kw, const rd_kw_type *add_kw);
void rd_kw_inplace_add(rd_kw_type *target_kw, const rd_kw_type *add_kw);
void rd_kw_inplace_sub(rd_kw_type *target_kw, const rd_kw_type *sub_kw);
void rd_kw_inplace_div(rd_kw_type *target_kw, const rd_kw_type *div_kw);
void rd_kw_inplace_mul(rd_kw_type *target_kw, const rd_kw_type *mul_kw);
void rd_kw_inplace_abs(rd_kw_type *kw);

void rd_kw_inplace_add_indexed(rd_kw_type *target_kw,
                               const std::vector<int> &index_set,
                               const rd_kw_type *add_kw);
void rd_kw_inplace_sub_indexed(rd_kw_type *target_kw,
                               const std::vector<int> &index_set,
                               const rd_kw_type *sub_kw);
void rd_kw_inplace_mul_indexed(rd_kw_type *target_kw,
                               const std::vector<int> &index_set,
                               const rd_kw_type *mul_kw);
void rd_kw_inplace_div_indexed(rd_kw_type *target_kw,
                               const std::vector<int> &index_set,
                               const rd_kw_type *div_kw);
void rd_kw_copy_indexed(rd_kw_type *target_kw,
                        const std::vector<int> &index_set,
                        const rd_kw_type *src_kw);

bool rd_kw_assert_binary_numeric(const rd_kw_type *kw1, const rd_kw_type *kw2);
bool rd_kw_assert_numeric(const rd_kw_type *kw);
bool rd_kw_assert_binary(const rd_kw_type *kw1, const rd_kw_type *kw2);

#define RD_KW_ASSERT_TYPED_BINARY_OP_HEADER(ctype)                             \
    bool rd_kw_assert_binary_##ctype(const rd_kw_type *kw1,                    \
                                     const rd_kw_type *kw2)
RD_KW_ASSERT_TYPED_BINARY_OP_HEADER(int);
RD_KW_ASSERT_TYPED_BINARY_OP_HEADER(float);
RD_KW_ASSERT_TYPED_BINARY_OP_HEADER(double);
#undef RD_KW_ASSERT_TYPED_BINARY_OP_HEADER

#define RD_KW_SCALE_TYPED_HEADER(ctype)                                        \
    void rd_kw_scale_##ctype(rd_kw_type *rd_kw, ctype scale_factor)
RD_KW_SCALE_TYPED_HEADER(int);
RD_KW_SCALE_TYPED_HEADER(float);
RD_KW_SCALE_TYPED_HEADER(double);
#undef RD_KW_SCALE_TYPED_HEADER
void rd_kw_scale_float_or_double(rd_kw_type *rd_kw, double scale_factor);

#define RD_KW_SHIFT_TYPED_HEADER(ctype)                                        \
    void rd_kw_shift_##ctype(rd_kw_type *rd_kw, ctype shift_factor)
RD_KW_SHIFT_TYPED_HEADER(int);
RD_KW_SHIFT_TYPED_HEADER(float);
RD_KW_SHIFT_TYPED_HEADER(double);
#undef RD_KW_SHIFT_TYPED_HEADER
void rd_kw_shift_float_or_double(rd_kw_type *rd_kw, double shift_value);

#define RD_KW_IGET_TYPED_HEADER(type)                                          \
    type rd_kw_iget_##type(const rd_kw_type *, int)
RD_KW_IGET_TYPED_HEADER(double);
RD_KW_IGET_TYPED_HEADER(float);
RD_KW_IGET_TYPED_HEADER(int);
#undef RD_KW_IGET_TYPED_HEADER
bool rd_kw_iget_bool(const rd_kw_type *rd_kw, int i);

#define RD_KW_ISET_TYPED_HEADER(type)                                          \
    void rd_kw_iset_##type(rd_kw_type *, int, type)
RD_KW_ISET_TYPED_HEADER(double);
RD_KW_ISET_TYPED_HEADER(float);
RD_KW_ISET_TYPED_HEADER(int);
#undef RD_KW_ISET_TYPED_HEADER
void rd_kw_iset_bool(rd_kw_type *rd_kw, int i, bool bool_value);

#define RD_KW_GET_TYPED_PTR_HEADER(type)                                       \
    type *rd_kw_get_##type##_ptr(const rd_kw_type *)
RD_KW_GET_TYPED_PTR_HEADER(double);
RD_KW_GET_TYPED_PTR_HEADER(float);
RD_KW_GET_TYPED_PTR_HEADER(int);
RD_KW_GET_TYPED_PTR_HEADER(bool);
#undef RD_KW_GET_TYPED_PTR_HEADER

#define RD_KW_SET_INDEXED_HEADER(ctype)                                        \
    void rd_kw_set_indexed_##ctype(                                            \
        rd_kw_type *rd_kw, const std::vector<int> &index_list, ctype value)
RD_KW_SET_INDEXED_HEADER(double);
RD_KW_SET_INDEXED_HEADER(float);
RD_KW_SET_INDEXED_HEADER(int);
#undef RD_KW_SET_INDEXED_HEADER

#define RD_KW_SHIFT_INDEXED_HEADER(ctype)                                      \
    void rd_kw_shift_indexed_##ctype(                                          \
        rd_kw_type *rd_kw, const std::vector<int> &index_list, ctype shift)
RD_KW_SHIFT_INDEXED_HEADER(int);
RD_KW_SHIFT_INDEXED_HEADER(float);
RD_KW_SHIFT_INDEXED_HEADER(double);
#undef RD_KW_SHIFT_INDEXED_HEADER

#define RD_KW_SCALE_INDEXED_HEADER(ctype)                                      \
    void rd_kw_scale_indexed_##ctype(                                          \
        rd_kw_type *rd_kw, const std::vector<int> &index_list, ctype scale)
RD_KW_SCALE_INDEXED_HEADER(int);
RD_KW_SCALE_INDEXED_HEADER(float);
RD_KW_SCALE_INDEXED_HEADER(double);
#undef RD_KW_SCALE_INDEXED_HEADER

#define RD_KW_MAX_MIN_HEADER(ctype)                                            \
    void rd_kw_max_min_##ctype(const rd_kw_type *rd_kw, ctype *_max,           \
                               ctype *_min)
RD_KW_MAX_MIN_HEADER(int);
RD_KW_MAX_MIN_HEADER(float);
RD_KW_MAX_MIN_HEADER(double);
#undef RD_KW_MAX_MIN_HEADER

void rd_kw_fix_uninitialized(rd_kw_type *rd_kw, int nx, int ny, int nz,
                             const int *actnum);

rd_type_enum rd_kw_get_type(const rd_kw_type *);

inline rd_kw_ptr make_rd_kw(const char *header, int size,
                            rd_data_type data_type,
                            const void *data = nullptr) {
    return std::make_unique<rd_kw_struct>(header, size, data_type, data);
}

inline std::string rd_kw_iget_stripped_string(const rd_kw_type *kw, int index) {
    const char *raw = static_cast<const char *>(rd_kw_iget_ptr(kw, index));
    const size_t width = rd_type_get_sizeof_iotype(rd_kw_get_data_type(kw));
    size_t len = 0;
    while (len < width && raw[len] != '\0')
        len++;
    return rd::strip_spaces(std::string(raw, len));
}
