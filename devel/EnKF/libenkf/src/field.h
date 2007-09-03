#ifndef __FIELD_H__
#define __FIELD_H__
#include <fortio.h>
#include <ecl_kw.h>
#include <config.h>
#include <enkf_util.h>
#include <field_config.h>

typedef struct field_struct field_type;

int          field_get_global_index(const field_type * , int  , int  , int );
double       field_ijk_get(const field_type * , int , int  , int );
bool         field_ijk_valid(const field_type * , int , int , int );
double       field_ijk_get_if_valid(const field_type * , int  , int  , int , bool *);
void 	     field_ecl_write(const field_type * , const char * );
void 	     field_ecl_write_fortio(const field_type * , fortio_type * , bool , bool , ecl_type_enum);
void 	     field_get_ecl_kw_data(field_type * , const ecl_kw_type * );
field_type * field_alloc(const field_config_type * );
void         field_free(field_type *);
double       field_ijk_lookup(const field_type * , int , int , int);
void         field_get_dims(const field_type *, int *, int *, int *);


MATH_OPS_HEADER(field);
VOID_ALLOC_HEADER(field);
VOID_FREE_HEADER(field);
VOID_FREE_DATA_HEADER(field);
VOID_REALLOC_DATA_HEADER(field);
VOID_COPYC_HEADER      (field);
VOID_ALLOC_ENSFILE_HEADER(field);
VOID_SWAPIN_HEADER(field)
VOID_SWAPOUT_HEADER(field)
VOID_SERIALIZE_HEADER  (field);

VOID_ECL_WRITE_HEADER (field)
VOID_ENS_WRITE_HEADER (field)
VOID_ENS_READ_HEADER  (field)


VOID_FUNC_HEADER       (field_sample   );
VOID_FUNC_HEADER       (field_isqrt    );



#endif
