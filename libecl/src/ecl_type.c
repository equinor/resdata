/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'ecl_type.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdlib.h>
#include <string.h>

#include <ert/util/util.h>
#include <ert/ecl/ecl_type.h>

/*****************************************************************/
/* The string names for the different ECLIPSE low-level
   types.
*/
#define ECL_TYPE_NAME_CHAR     "CHAR"
#define ECL_TYPE_NAME_C010     "C010"
#define ECL_TYPE_NAME_FLOAT    "REAL"
#define ECL_TYPE_NAME_INT      "INTE"
#define ECL_TYPE_NAME_DOUBLE   "DOUB"
#define ECL_TYPE_NAME_BOOL     "LOGI"
#define ECL_TYPE_NAME_MESSAGE  "MESS"

ecl_data_type * ecl_type_alloc_copy(const ecl_data_type * src_type) {
    ecl_data_type * ecl_type = util_malloc(sizeof * ecl_type);
    memcpy(ecl_type, src_type, sizeof * ecl_type);
    return ecl_type;
}

void ecl_type_free(ecl_data_type * ecl_type) {
    if(ecl_type == NULL)
        return;

    free(ecl_type);
}

ecl_data_type ecl_type_get_data_type(const ecl_type_enum type, const int length) {
    ecl_data_type * ecl_type = NULL;
    switch(type) {
    case(ECL_CHAR_TYPE):
      ecl_type = &ECL_CHAR;
      break;
    case(ECL_INT_TYPE):
      ecl_type = &ECL_INT;
      break;
    case(ECL_FLOAT_TYPE):
      ecl_type = &ECL_FLOAT;
      break;
    case(ECL_DOUBLE_TYPE):
      ecl_type = &ECL_DOUBLE;
      break;
    case(ECL_BOOL_TYPE):
      ecl_type = &ECL_BOOL;
      break;
    case(ECL_MESS_TYPE):
      ecl_type = &ECL_MESS;
      break;
    default:
      util_abort("%s: invalid ecl_type:(%d, %d)\n", __func__, type, length);
    }

    if(ecl_type->length != length)
        util_abort(
                "%s: length mismatch for type %d, was: %d, expected: %d\n",
                __func__, type, length, ecl_type->length);   

    return *ecl_type;
}

ecl_data_type ecl_type_get_data_type_from_type(const ecl_type_enum type) {
    ecl_data_type * ecl_type = NULL;
    switch(type) {
    case(ECL_CHAR_TYPE):
      ecl_type = &ECL_CHAR;
      break;
    case(ECL_INT_TYPE):
      ecl_type = &ECL_INT;
      break;
    case(ECL_FLOAT_TYPE):
      ecl_type = &ECL_FLOAT;
      break;
    case(ECL_DOUBLE_TYPE):
      ecl_type = &ECL_DOUBLE;
      break;
    case(ECL_BOOL_TYPE):
      ecl_type = &ECL_BOOL;
      break;
    case(ECL_MESS_TYPE):
      ecl_type = &ECL_MESS;
      break;
    default:
      util_abort("%s: invalid ecl_type: %d\n", __func__, type);
    }

    return *ecl_type;
}

const char * ecl_type_get_type_name(const ecl_data_type * ecl_type) {
  switch (ecl_type->type) {
  case(ECL_CHAR_TYPE):
    return ECL_TYPE_NAME_CHAR ;
  case(ECL_C010_TYPE):
    return ECL_TYPE_NAME_C010;
  case(ECL_FLOAT_TYPE):
    return ECL_TYPE_NAME_FLOAT;
  case(ECL_DOUBLE_TYPE):
    return ECL_TYPE_NAME_DOUBLE;
  case(ECL_INT_TYPE):
    return ECL_TYPE_NAME_INT;
  case(ECL_BOOL_TYPE):
    return ECL_TYPE_NAME_BOOL;
  case(ECL_MESS_TYPE):
    return ECL_TYPE_NAME_MESSAGE;
  default:
    util_abort("Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_type->type);
  }
  return NULL; /* Dummy */
}

ecl_data_type ecl_type_get_type_from_name( const char * type_name ) {
  if (strncmp( type_name , ECL_TYPE_NAME_FLOAT , ECL_TYPE_LENGTH) == 0)
    return ECL_FLOAT;
  else if (strncmp( type_name , ECL_TYPE_NAME_INT , ECL_TYPE_LENGTH) == 0)
    return ECL_INT;
  else if (strncmp( type_name , ECL_TYPE_NAME_DOUBLE , ECL_TYPE_LENGTH) == 0)
    return ECL_DOUBLE;
  else if (strncmp( type_name , ECL_TYPE_NAME_CHAR , ECL_TYPE_LENGTH) == 0)
    return ECL_CHAR;
  else if (strncmp( type_name , ECL_TYPE_NAME_C010 , ECL_TYPE_LENGTH) == 0)
    return ECL_C010;
  else if (strncmp( type_name , ECL_TYPE_NAME_MESSAGE , ECL_TYPE_LENGTH) == 0)
    return ECL_MESS;
  else if (strncmp( type_name , ECL_TYPE_NAME_BOOL , ECL_TYPE_LENGTH) == 0)
    return ECL_BOOL;
  else {
    util_abort("%s: unrecognized type name:%s \n",__func__ , type_name);
    return ECL_INT; /* Dummy */
  }
}


int ecl_type_get_sizeof_ctype_fortio(const ecl_data_type * ecl_type) {
  int size = ecl_type_get_sizeof_ctype ( ecl_type );
  if (ecl_type->type == ECL_CHAR_TYPE)
    size = ECL_STRING8_LENGTH  * sizeof(char);

  if (ecl_type->type == ECL_C010_TYPE)
    size = ECL_STRING10_LENGTH  * sizeof(char);

  return size;
}

int ecl_type_get_sizeof_ctype(const ecl_data_type * ecl_type) {
  int sizeof_ctype = -1;
  switch (ecl_type->type) {
  case(ECL_CHAR_TYPE):
    /*
       One element of character data is a string section of 8
       characters + \0.  Observe that the return value here
       corresponds to the size requirements of ECL_CHAR_TYPE instance
       in memory; on disk the trailing \0 is not stored.
    */
    sizeof_ctype = (ECL_STRING8_LENGTH + 1) * sizeof(char);
    break;
  case(ECL_C010_TYPE):
    /*
       One element of character data is a string section of 8
       characters + \0.  Observe that the return value here
       corresponds to the size requirements of ECL_CHAR_TYPE instance
       in memory; on disk the trailing \0 is not stored.
    */
    sizeof_ctype = (ECL_STRING10_LENGTH + 1) * sizeof(char);
    break;
  case(ECL_FLOAT_TYPE):
    sizeof_ctype = sizeof(float);
    break;
  case(ECL_DOUBLE_TYPE):
    sizeof_ctype = sizeof(double);
    break;
  case(ECL_INT_TYPE):
    sizeof_ctype = sizeof(int);
    break;
  case(ECL_BOOL_TYPE):
    sizeof_ctype = sizeof(int); // The ECL_BOOL_TYPE type is internally implemented as an integer - and not a bool.
    break;
  case(ECL_MESS_TYPE):
    sizeof_ctype = sizeof(char);
    break;
  default:
    util_abort("Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_type->type);
  }
  return sizeof_ctype;
}

bool ecl_type_is_numeric(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_INT_TYPE ||
            ecl_type->type == ECL_FLOAT_TYPE ||
            ecl_type->type == ECL_DOUBLE_TYPE);
}

bool ecl_type_is_equal(const ecl_data_type * ecl_type1, const ecl_data_type * ecl_type2) {
    return (ecl_type1->type == ecl_type2->type && ecl_type1->length == ecl_type2->length);
}

bool ecl_type_is_char(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_CHAR_TYPE);
}

bool ecl_type_is_int(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_INT_TYPE);
}

bool ecl_type_is_float(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_FLOAT_TYPE);
}

bool ecl_type_is_double(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_DOUBLE_TYPE);
}

bool ecl_type_is_mess(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_MESS_TYPE);
}
 
bool ecl_type_is_bool(const ecl_data_type * ecl_type) {
    return (ecl_type->type == ECL_BOOL_TYPE);
}
