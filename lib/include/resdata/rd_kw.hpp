#ifndef ERT_RD_KW_H
#define ERT_RD_KW_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <ert/util/buffer.hpp>
#include <ert/util/type_macros.hpp>

#include <resdata/fortio.h>
#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>

UTIL_IS_INSTANCE_HEADER(rd_kw);

typedef struct rd_kw_struct rd_kw_type;

typedef enum { RD_KW_READ_OK = 0, RD_KW_READ_FAIL = 1 } rd_read_status_enum;

/*
  The size of an rd_kw instance is denoted with an integer. The
  choice of int to store the size obviously limits the maximum size to
  INT_MAX elements. This choice is an historical mistake - it should
  probably have been size_t; however the rd_kw datastructure is
  tightly bound to the on-disk binary format supplied by Eclipse, and
  there the number of elements is stored as a signed(?) 32 bit
  integer - so using int for size does make some sense-
*/

#define RD_KW_MAX_SIZE INT_MAX

/*
  Character data in restart format files comes as an array of fixed-length
  string. Each of these strings is 8 characters long. The type name,
  i.e. 'REAL', 'INTE', ... , come as 4 character strings.
*/
#define RD_KW_HEADER_DATA_SIZE RD_STRING8_LENGTH + RD_TYPE_LENGTH + 4
#define RD_KW_HEADER_FORTIO_SIZE RD_KW_HEADER_DATA_SIZE + 8

int rd_kw_first_different(const rd_kw_type *kw1, const rd_kw_type *kw2,
                          int offset, double abs_epsilon, double rel_epsilon);
size_t rd_kw_fortio_size(const rd_kw_type *rd_kw);
void *rd_kw_get_ptr(const rd_kw_type *rd_kw);
void rd_kw_set_data_ptr(rd_kw_type *rd_kw, void *data);
void rd_kw_fwrite_data(const rd_kw_type *_rd_kw, fortio_type *fortio);
bool rd_kw_fread_realloc_data(rd_kw_type *rd_kw, fortio_type *fortio);
rd_data_type rd_kw_get_data_type(const rd_kw_type *);
const char *rd_kw_get_header8(const rd_kw_type *);
const char *rd_kw_get_header(const rd_kw_type *rd_kw);
rd_kw_type *rd_kw_alloc_empty(void);
rd_read_status_enum rd_kw_fread_header(rd_kw_type *, fortio_type *);
void rd_kw_set_header_name(rd_kw_type *, const char *);
bool rd_kw_fseek_kw(const char *, bool, bool, fortio_type *);
bool rd_kw_fseek_last_kw(const char *, bool, fortio_type *);
void rd_kw_inplace_update_file(const rd_kw_type *, const char *, int);
void rd_kw_fskip(fortio_type *);
void rd_kw_alloc_data(rd_kw_type *);
void rd_kw_alloc_double_data(rd_kw_type *rd_kw, double *values);
void rd_kw_alloc_float_data(rd_kw_type *rd_kw, float *values);
bool rd_kw_fread_realloc(rd_kw_type *, fortio_type *);
void rd_kw_fread(rd_kw_type *, fortio_type *);
rd_kw_type *rd_kw_fread_alloc(fortio_type *);
rd_kw_type *rd_kw_alloc_actnum(const rd_kw_type *porv_kw, float porv_limit);
rd_kw_type *rd_kw_alloc_actnum_bitmask(const rd_kw_type *porv_kw,
                                       float porv_limit, int actnum_bitmask);
void rd_kw_free_data(rd_kw_type *);
void rd_kw_fread_indexed_data(fortio_type *fortio, offset_type data_offset,
                              rd_data_type, int element_count,
                              const int_vector_type *index_map, char *buffer);
void rd_kw_free(rd_kw_type *);
void rd_kw_free__(void *);
rd_kw_type *rd_kw_alloc_copy(const rd_kw_type *);
rd_kw_type *rd_kw_alloc_sub_copy(const rd_kw_type *src, const char *new_kw,
                                 int offset, int count);
const void *rd_kw_copyc__(const void *);
rd_kw_type *rd_kw_alloc_slice_copy(const rd_kw_type *src, int index1,
                                   int index2, int stride);
void rd_kw_resize(rd_kw_type *rd_kw, int new_size);
//void        * rd_kw_get_data_ref(const rd_kw_type *);
void *rd_kw_alloc_data_copy(const rd_kw_type *);
void rd_kw_memcpy(rd_kw_type *, const rd_kw_type *);
void rd_kw_get_memcpy_data(const rd_kw_type *, void *);
void rd_kw_get_memcpy_float_data(const rd_kw_type *rd_kw, float *target);
void rd_kw_get_memcpy_double_data(const rd_kw_type *rd_kw, double *target);
void rd_kw_get_memcpy_int_data(const rd_kw_type *rd_kw, int *target);
void rd_kw_set_memcpy_data(rd_kw_type *, const void *);
bool rd_kw_fwrite(const rd_kw_type *, fortio_type *);
void rd_kw_iget(const rd_kw_type *, int, void *);
void rd_kw_iset(rd_kw_type *rd_kw, int i, const void *iptr);
void rd_kw_iset_char_ptr(rd_kw_type *rd_kw, int index, const char *s);
void rd_kw_iset_string8(rd_kw_type *rd_kw, int index, const char *s8);
void rd_kw_iset_string_ptr(rd_kw_type *, int, const char *);
const char *rd_kw_iget_string_ptr(const rd_kw_type *, int);
const char *rd_kw_iget_char_ptr(const rd_kw_type *rd_kw, int i);
void *rd_kw_iget_ptr(const rd_kw_type *, int);
int rd_kw_get_size(const rd_kw_type *);
bool rd_kw_ichar_eq(const rd_kw_type *, int, const char *);
rd_kw_type *rd_kw_alloc(const char *header, int size, rd_data_type);
rd_kw_type *rd_kw_alloc_new(const char *, int, rd_data_type, const void *);
rd_kw_type *rd_kw_alloc_new_shared(const char *, int, rd_data_type, void *);
rd_kw_type *rd_kw_alloc_global_copy(const rd_kw_type *src,
                                    const rd_kw_type *actnum);
void rd_kw_fwrite_param(const char *, bool, const char *, rd_data_type, int,
                        void *);
void rd_kw_fwrite_param_fortio(fortio_type *, const char *, rd_data_type, int,
                               void *);
void rd_kw_summarize(const rd_kw_type *rd_kw);
void rd_kw_fread_double_param(const char *, bool, double *);
float rd_kw_iget_as_float(const rd_kw_type *rd_kw, int i);
double rd_kw_iget_as_double(const rd_kw_type *rd_kw, int i);
void rd_kw_get_data_as_double(const rd_kw_type *, double *);
void rd_kw_get_data_as_float(const rd_kw_type *rd_kw, float *float_data);
bool rd_kw_name_equal(const rd_kw_type *rd_kw, const char *name);
bool rd_kw_header_eq(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2);
bool rd_kw_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2);
bool rd_kw_size_and_type_equal(const rd_kw_type *rd_kw1,
                               const rd_kw_type *rd_kw2);
bool rd_kw_icmp_string(const rd_kw_type *rd_kw, int index,
                       const char *other_string);
bool rd_kw_numeric_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2,
                         double abs_diff, double rel_diff);
bool rd_kw_block_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2,
                       int cmp_elements);
bool rd_kw_data_equal(const rd_kw_type *rd_kw, const void *data);
bool rd_kw_content_equal(const rd_kw_type *rd_kw1, const rd_kw_type *rd_kw2);
bool rd_kw_fskip_data__(rd_data_type, int, fortio_type *);
bool rd_kw_fskip_data(rd_kw_type *rd_kw, fortio_type *fortio);
bool rd_kw_fread_data(rd_kw_type *rd_kw, fortio_type *fortio);
void rd_kw_fskip_header(fortio_type *fortio);
bool rd_kw_size_and_numeric_type_equal(const rd_kw_type *kw1,
                                       const rd_kw_type *kw2);
bool rd_kw_inplace_safe_div(rd_kw_type *target_kw, const rd_kw_type *divisor);
void rd_kw_inplace_sqrt(rd_kw_type *kw);

bool rd_kw_is_kw_file(fortio_type *fortio);

int rd_kw_element_sum_int(const rd_kw_type *rd_kw);
double rd_kw_element_sum_float(const rd_kw_type *rd_kw);
void rd_kw_inplace_inv(rd_kw_type *my_kw);
void rd_kw_element_sum(const rd_kw_type *, void *);
void rd_kw_element_sum_indexed(const rd_kw_type *rd_kw,
                               const int_vector_type *index_list, void *_sum);
void rd_kw_max_min(const rd_kw_type *, void *, void *);
void *rd_kw_get_void_ptr(const rd_kw_type *rd_kw);

rd_kw_type *rd_kw_buffer_alloc(buffer_type *buffer);
void rd_kw_buffer_store(const rd_kw_type *rd_kw, buffer_type *buffer);

void rd_kw_fprintf_data(const rd_kw_type *rd_kw, const char *fmt, FILE *stream);
void rd_kw_memcpy_data(rd_kw_type *target, const rd_kw_type *src);

bool rd_kw_assert_numeric(const rd_kw_type *kw);
bool rd_kw_assert_binary(const rd_kw_type *kw1, const rd_kw_type *kw2);

void rd_kw_scalar_set_bool(rd_kw_type *rd_kw, bool bool_value);
void rd_kw_scalar_set__(rd_kw_type *rd_kw, const void *value);
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
                               const int_vector_type *index_set,
                               const rd_kw_type *add_kw);
void rd_kw_inplace_sub_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *sub_kw);
void rd_kw_inplace_mul_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *mul_kw);
void rd_kw_inplace_div_indexed(rd_kw_type *target_kw,
                               const int_vector_type *index_set,
                               const rd_kw_type *div_kw);
void rd_kw_copy_indexed(rd_kw_type *target_kw, const int_vector_type *index_set,
                        const rd_kw_type *src_kw);

bool rd_kw_assert_binary_numeric(const rd_kw_type *kw1, const rd_kw_type *kw2);
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
        rd_kw_type *rd_kw, const int_vector_type *index_list, ctype value)
RD_KW_SET_INDEXED_HEADER(double);
RD_KW_SET_INDEXED_HEADER(float);
RD_KW_SET_INDEXED_HEADER(int);
#undef RD_KW_SET_INDEXED_HEADER

#define RD_KW_SHIFT_INDEXED_HEADER(ctype)                                      \
    void rd_kw_shift_indexed_##ctype(                                          \
        rd_kw_type *rd_kw, const int_vector_type *index_list, ctype shift)
RD_KW_SHIFT_INDEXED_HEADER(int);
RD_KW_SHIFT_INDEXED_HEADER(float);
RD_KW_SHIFT_INDEXED_HEADER(double);
#undef RD_KW_SHIFT_INDEXED_HEADER

#define RD_KW_SCALE_INDEXED_HEADER(ctype)                                      \
    void rd_kw_scale_indexed_##ctype(                                          \
        rd_kw_type *rd_kw, const int_vector_type *index_list, ctype scale)
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

#include <resdata/rd_kw_grdecl.hpp>

#ifdef __cplusplus
}
#endif
#endif
