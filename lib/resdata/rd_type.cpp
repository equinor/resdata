#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ert/util/util.hpp>
#include <resdata/rd_type.hpp>

#define RD_TYPE_NAME_CHAR "CHAR"
#define RD_TYPE_NAME_FLOAT "REAL"
#define RD_TYPE_NAME_INT "INTE"
#define RD_TYPE_NAME_DOUBLE "DOUB"
#define RD_TYPE_NAME_BOOL "LOGI"
#define RD_TYPE_NAME_MESSAGE "MESS"

static char *alloc_string_name(const rd_data_type rd_type) {
    return util_alloc_sprintf("C%03d", rd_type_get_sizeof_iotype(rd_type));
}

static bool is_rd_string_name(const char *type_name) {
    return (type_name[0] == 'C' && isdigit(type_name[1]) &&
            isdigit(type_name[2]) && isdigit(type_name[3]));
}

static size_t get_rd_string_length(const char *type_name) {
    if (!is_rd_string_name(type_name))
        util_abort("%s: Expected eclipse string (CXXX), received %s\n",
                   __func__, type_name);

    return atoi(type_name + 1);
}

rd_data_type rd_type_create(const rd_type_enum type,
                            const size_t element_size) {
    rd_data_type rd_type =
        (type == RD_STRING_TYPE ? RD_STRING(element_size)
                                : rd_type_create_from_type(type));

    if ((size_t)rd_type_get_sizeof_iotype(rd_type) != element_size)
        util_abort(
            "%s: element_size mismatch for type %d, was: %d, expected: %d\n",
            __func__, type, element_size, rd_type_get_sizeof_iotype(rd_type));

    return rd_type;
}

rd_data_type rd_type_create_from_type(const rd_type_enum type) {
    switch (type) {
    case (RD_CHAR_TYPE):
        return RD_CHAR;
    case (RD_INT_TYPE):
        return RD_INT;
    case (RD_FLOAT_TYPE):
        return RD_FLOAT;
    case (RD_DOUBLE_TYPE):
        return RD_DOUBLE;
    case (RD_BOOL_TYPE):
        return RD_BOOL;
    case (RD_MESS_TYPE):
        return RD_MESS;
    case (RD_STRING_TYPE):
        util_abort("%s: Variable length string type cannot be created"
                   " from type alone!\n",
                   __func__);
        return RD_STRING(0); /* Dummy */
    default:
        util_abort("%s: invalid rd_type: %d\n", __func__, type);
        return RD_INT; /* Dummy */
    }
}

rd_type_enum rd_type_get_type(const rd_data_type rd_type) {
    return rd_type.type;
}

char *rd_type_alloc_name(const rd_data_type rd_type) {
    switch (rd_type.type) {
    case (RD_CHAR_TYPE):
        return util_alloc_string_copy(RD_TYPE_NAME_CHAR);
    case (RD_STRING_TYPE):
        return alloc_string_name(rd_type);
    case (RD_FLOAT_TYPE):
        return util_alloc_string_copy(RD_TYPE_NAME_FLOAT);
    case (RD_DOUBLE_TYPE):
        return util_alloc_string_copy(RD_TYPE_NAME_DOUBLE);
    case (RD_INT_TYPE):
        return util_alloc_string_copy(RD_TYPE_NAME_INT);
    case (RD_BOOL_TYPE):
        return util_alloc_string_copy(RD_TYPE_NAME_BOOL);
    case (RD_MESS_TYPE):
        return util_alloc_string_copy(RD_TYPE_NAME_MESSAGE);
    default:
        util_abort("Internal error in %s - internal eclipse_type: %d not "
                   "recognized - aborting \n",
                   __func__, rd_type.type);
        return NULL; /* Dummy */
    }
}

rd_data_type rd_type_create_from_name(const char *type_name) {
    if (strncmp(type_name, RD_TYPE_NAME_FLOAT, RD_TYPE_LENGTH) == 0)
        return RD_FLOAT;
    else if (strncmp(type_name, RD_TYPE_NAME_INT, RD_TYPE_LENGTH) == 0)
        return RD_INT;
    else if (strncmp(type_name, RD_TYPE_NAME_DOUBLE, RD_TYPE_LENGTH) == 0)
        return RD_DOUBLE;
    else if (strncmp(type_name, RD_TYPE_NAME_CHAR, RD_TYPE_LENGTH) == 0)
        return RD_CHAR;
    else if (is_rd_string_name(type_name))
        return RD_STRING(get_rd_string_length(type_name));
    else if (strncmp(type_name, RD_TYPE_NAME_MESSAGE, RD_TYPE_LENGTH) == 0)
        return RD_MESS;
    else if (strncmp(type_name, RD_TYPE_NAME_BOOL, RD_TYPE_LENGTH) == 0)
        return RD_BOOL;
    else {
        util_abort("%s: unrecognized type name:%s \n", __func__, type_name);
        return RD_INT; /* Dummy */
    }
}

int rd_type_get_sizeof_ctype(const rd_data_type rd_type) {
    return rd_type.element_size;
}

int rd_type_get_sizeof_iotype(const rd_data_type rd_type) {
    if (rd_type_is_bool(rd_type))
        return sizeof(int);

    if (rd_type_is_char(rd_type))
        return rd_type.element_size - 1;

    if (rd_type_is_string(rd_type))
        return rd_type.element_size - 1;

    return rd_type.element_size;
}

bool rd_type_is_numeric(const rd_data_type rd_type) {
    return (rd_type_is_int(rd_type) || rd_type_is_float(rd_type) ||
            rd_type_is_double(rd_type));
}

bool rd_type_is_alpha(const rd_data_type rd_type) {
    return (rd_type_is_char(rd_type) || rd_type_is_mess(rd_type) ||
            rd_type_is_string(rd_type));
}

bool rd_type_is_equal(const rd_data_type rd_type1,
                      const rd_data_type rd_type2) {
    return (rd_type1.type == rd_type2.type &&
            rd_type1.element_size == rd_type2.element_size);
}

bool rd_type_is_char(const rd_data_type rd_type) {
    return (rd_type.type == RD_CHAR_TYPE);
}

bool rd_type_is_int(const rd_data_type rd_type) {
    return (rd_type.type == RD_INT_TYPE);
}

bool rd_type_is_float(const rd_data_type rd_type) {
    return (rd_type.type == RD_FLOAT_TYPE);
}

bool rd_type_is_double(const rd_data_type rd_type) {
    return (rd_type.type == RD_DOUBLE_TYPE);
}

bool rd_type_is_mess(const rd_data_type rd_type) {
    return (rd_type.type == RD_MESS_TYPE);
}

bool rd_type_is_bool(const rd_data_type rd_type) {
    return (rd_type.type == RD_BOOL_TYPE);
}

bool rd_type_is_string(const rd_data_type rd_type) {
    return (rd_type.type == RD_STRING_TYPE);
}
