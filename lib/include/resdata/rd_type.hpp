#ifndef ERT_RD_TYPE_H
#define ERT_RD_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

/*
  The type of an eclipse keyword is carried by a struct
  rd_type_struct which contains a type enum, and the size in bytes of
  one such element. These structs are for the most part handled with
  value semantics, and created with macros RD_INT, RD_FLOAT and so
  on.

  The macros in C use designated initializers, whereas the C++ macros
  use a constructor, for this reason this file has two slightly
  different code paths for C and C++.
*/

#define RD_STRING8_LENGTH 8
#define RD_TYPE_LENGTH 4

typedef enum {
    RD_CHAR_TYPE = 0,
    RD_FLOAT_TYPE = 1,
    RD_DOUBLE_TYPE = 2,
    RD_INT_TYPE = 3,
    RD_BOOL_TYPE = 4,
    RD_MESS_TYPE = 5,
    RD_STRING_TYPE = 7
} rd_type_enum;

#define RD_TYPE_ENUM_DEFS                                                      \
    {.value = 0, .name = "RD_CHAR_TYPE"},                                      \
        {.value = 1, .name = "RD_FLOAT_TYPE"},                                 \
        {.value = 2, .name = "RD_DOUBLE_TYPE"},                                \
        {.value = 3, .name = "RD_INT_TYPE"},                                   \
        {.value = 4, .name = "RD_BOOL_TYPE"},                                  \
        {.value = 5, .name = "RD_MESS_TYPE"}, {                                \
        .value = 7, .name = "RD_STRING_TYPE"                                   \
    }

/*
  Character data in restart format files comes as an array of fixed-length
  string. Each of these strings is 8 characters long. The type name,
  i.e. 'REAL', 'INTE', ... , come as 4 character strings.
*/

#define RD_STRING8_LENGTH 8 // 'Normal' 8 characters 'CHAR' type.
#define RD_TYPE_LENGTH 4

struct rd_type_struct {
    const rd_type_enum type;
    const size_t element_size;
};

#ifdef __cplusplus

#define RD_INT                                                                 \
    rd_data_type { RD_INT_TYPE, sizeof(int) }
#define RD_FLOAT                                                               \
    rd_data_type { RD_FLOAT_TYPE, sizeof(float) }
#define RD_DOUBLE                                                              \
    rd_data_type { RD_DOUBLE_TYPE, sizeof(double) }
#define RD_BOOL                                                                \
    rd_data_type { RD_BOOL_TYPE, sizeof(bool) }
#define RD_CHAR                                                                \
    rd_data_type { RD_CHAR_TYPE, RD_STRING8_LENGTH + 1 }
#define RD_MESS                                                                \
    rd_data_type { RD_MESS_TYPE, 0 }
#define RD_STRING(size)                                                        \
    rd_data_type { RD_STRING_TYPE, (size) + 1 }
}

#else

#define RD_CHAR                                                                \
    (rd_data_type) {                                                           \
        .type = RD_CHAR_TYPE, .element_size = RD_STRING8_LENGTH + 1            \
    }
#define RD_INT                                                                 \
    (rd_data_type) { .type = RD_INT_TYPE, .element_size = sizeof(int) }
#define RD_FLOAT                                                               \
    (rd_data_type) { .type = RD_FLOAT_TYPE, .element_size = sizeof(float) }
#define RD_DOUBLE                                                              \
    (rd_data_type) { .type = RD_DOUBLE_TYPE, .element_size = sizeof(double) }
#define RD_BOOL                                                                \
    (rd_data_type) { .type = RD_BOOL_TYPE, .element_size = sizeof(bool) }
#define RD_MESS                                                                \
    (rd_data_type) { .type = RD_MESS_TYPE, .element_size = 0 }
#define RD_STRING(size)                                                        \
    (rd_data_type) { .type = RD_STRING_TYPE, .element_size = (size) + 1 }

#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_type_struct rd_data_type;

rd_data_type rd_type_create_from_name(const char *);
rd_data_type rd_type_create(const rd_type_enum, const size_t);
rd_data_type rd_type_create_from_type(const rd_type_enum);

rd_type_enum rd_type_get_type(const rd_data_type);
char *rd_type_alloc_name(const rd_data_type);

int rd_type_get_sizeof_ctype(const rd_data_type);
int rd_type_get_sizeof_iotype(const rd_data_type);

bool rd_type_is_equal(const rd_data_type, const rd_data_type);

bool rd_type_is_numeric(const rd_data_type);
bool rd_type_is_alpha(const rd_data_type);
bool rd_type_is_char(const rd_data_type);
bool rd_type_is_int(const rd_data_type);
bool rd_type_is_float(const rd_data_type);
bool rd_type_is_double(const rd_data_type);
bool rd_type_is_mess(const rd_data_type);
bool rd_type_is_bool(const rd_data_type);
bool rd_type_is_string(const rd_data_type);

// Temporary fixup for OPM.
char *rd_type_get_name(const rd_data_type);

#ifdef __cplusplus
}
#endif

#endif
