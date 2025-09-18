#include <cstring>

#include <resdata/rd_type.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_grdecl.hpp>

/**
 *
 * Functions only to be used by the *PYTHON* prototype for ResDataType
 *
 */

#ifdef __cplusplus
extern "C" {

static rd_data_type *rd_type_alloc_copy_python(const rd_data_type *src_type) {
    rd_data_type *data_type = (rd_data_type *)util_malloc(sizeof *src_type);
    memcpy(data_type, src_type, sizeof *data_type);
    return data_type;
}

rd_data_type *rd_type_alloc_python(const rd_type_enum type,
                                   const size_t element_size) {
    rd_data_type src_type = rd_type_create(type, element_size);
    return rd_type_alloc_copy_python(&src_type);
}

rd_data_type *rd_type_alloc_from_type_python(const rd_type_enum type) {
    rd_data_type src_type = rd_type_create_from_type(type);
    return rd_type_alloc_copy_python(&src_type);
}

rd_data_type *rd_type_alloc_from_name_python(const char *name) {
    rd_data_type src_type = rd_type_create_from_name(name);
    return rd_type_alloc_copy_python(&src_type);
}

void rd_type_free_python(rd_data_type *data_type) { free(data_type); }

rd_type_enum rd_type_get_type_python(const rd_data_type *rd_type) {
    return rd_type_get_type(*rd_type);
}

const char *rd_type_alloc_name_python(const rd_data_type *rd_type) {
    return rd_type_alloc_name(*rd_type);
}

int rd_type_get_sizeof_iotype_python(const rd_data_type *rd_type) {
    return rd_type_get_sizeof_iotype(*rd_type);
}

bool rd_type_is_numeric_python(const rd_data_type *rd_type) {
    return rd_type_is_numeric(*rd_type);
}

bool rd_type_is_equal_python(const rd_data_type *rd_type1,
                             const rd_data_type *rd_type2) {
    return rd_type_is_equal(*rd_type1, *rd_type2);
}

bool rd_type_is_char_python(const rd_data_type *rd_type) {
    return rd_type_is_char(*rd_type);
}

bool rd_type_is_int_python(const rd_data_type *rd_type) {
    return rd_type_is_int(*rd_type);
}

bool rd_type_is_float_python(const rd_data_type *rd_type) {
    return rd_type_is_float(*rd_type);
}

bool rd_type_is_double_python(const rd_data_type *rd_type) {
    return rd_type_is_double(*rd_type);
}

bool rd_type_is_mess_python(const rd_data_type *rd_type) {
    return rd_type_is_mess(*rd_type);
}

bool rd_type_is_bool_python(const rd_data_type *rd_type) {
    return rd_type_is_bool(*rd_type);
}

bool rd_type_is_string_python(const rd_data_type *rd_type) {
    return rd_type_is_string(*rd_type);
}

/**
 *
 * Functions for the ResdataKw prototype
 *
 */
rd_kw_type *rd_kw_fscanf_alloc_grdecl_dynamic_python(
    FILE *stream, const char *kw, bool strict, const rd_data_type *data_type) {
    return rd_kw_fscanf_alloc_grdecl_dynamic__(stream, kw, strict, *data_type);
}

rd_kw_type *rd_kw_alloc_python(const char *header, int size,
                               const rd_data_type *data_type) {
    return rd_kw_alloc(header, size, *data_type);
}

rd_data_type *rd_kw_get_data_type_python(const rd_kw_type *rd_kw) {
    rd_data_type data_type = rd_kw_get_data_type(rd_kw);
    return rd_type_alloc_copy_python(&data_type);
}
}
#endif
