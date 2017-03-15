/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'ecl_type.h' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#ifndef ERT_ECL_TYPE_H
#define ERT_ECL_TYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ert/ecl/ecl_util.h>

struct ecl_type_struct {
    const ecl_type_enum type;

    // Denotes the number of charachters used to represent
    // the data type in *MEMORY*
    const size_t element_size;
};

typedef struct ecl_type_struct ecl_data_type;


#define ECL_CHAR (ecl_data_type) {.type = ECL_CHAR_TYPE, .element_size = 8+1}
#define ECL_INT (ecl_data_type) {.type = ECL_INT_TYPE, .element_size = sizeof(int)/sizeof(char)}
#define ECL_FLOAT (ecl_data_type) {.type = ECL_FLOAT_TYPE, .element_size = sizeof(float)/sizeof(char)}
#define ECL_DOUBLE (ecl_data_type) {.type = ECL_DOUBLE_TYPE, .element_size = sizeof(double)/sizeof(char)}
#define ECL_BOOL (ecl_data_type) {.type = ECL_BOOL_TYPE, .element_size = sizeof(int)/sizeof(char)}
#define ECL_MESS (ecl_data_type) {.type = ECL_MESS_TYPE, .element_size = 1}
#define ECL_C010 (ecl_data_type) {.type = ECL_C010_TYPE, .element_size = 10+1}


ecl_data_type      ecl_type_create_data_type_from_name(const char *);
ecl_data_type      ecl_type_create_data_type(const ecl_type_enum, const size_t);
ecl_data_type      ecl_type_create_data_type_from_type(const ecl_type_enum);

ecl_type_enum      ecl_type_get_type(const ecl_data_type);
size_t             ecl_type_get_element_size(const ecl_data_type);
const char *       ecl_type_get_type_name(const ecl_data_type);

int                ecl_type_get_sizeof_ctype(const ecl_data_type);
int                ecl_type_get_sizeof_ctype_fortio(const ecl_data_type);

bool               ecl_type_is_equal(const ecl_data_type, const ecl_data_type);

bool               ecl_type_is_numeric(const ecl_data_type);
bool               ecl_type_is_char(const ecl_data_type);
bool               ecl_type_is_int(const ecl_data_type);
bool               ecl_type_is_float(const ecl_data_type);
bool               ecl_type_is_double(const ecl_data_type);
bool               ecl_type_is_mess(const ecl_data_type);
bool               ecl_type_is_bool(const ecl_data_type);
bool               ecl_type_is_C010(const ecl_data_type);

#ifdef __cplusplus
}
#endif
#endif
