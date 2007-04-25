#ifndef __ECL_KW_H__
#define __ECL_KW_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fortio.h>

#define ecl_str_len   8


typedef struct ecl_kw_struct      ecl_kw_type;
typedef enum   ecl_type_enum_def  ecl_type_enum;
enum ecl_type_enum_def {ecl_char_type , ecl_float_type , ecl_double_type , ecl_int_type , ecl_bool_type , ecl_mess_type};


ecl_type_enum ecl_kw_get_type(const ecl_kw_type *);
const char  * ecl_kw_get_str_type_ref(const ecl_kw_type *);
/*const char  * ecl_kw_str_type(ecl_type_enum );*/
void          ecl_kw_fread_data(ecl_kw_type *, fortio_type *);
const char  * ecl_kw_get_header_ref(const ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_empty(bool , bool);
void          ecl_kw_rewind(const ecl_kw_type * , fortio_type *);
void          ecl_kw_copy_data(const ecl_kw_type * , void *);
bool          ecl_kw_fread_header(ecl_kw_type *, fortio_type *);
void          ecl_kw_set_header(ecl_kw_type  * , const char * , int , const char *);
void          ecl_kw_set_header_alloc(ecl_kw_type  * , const char * , int , const char *);
void          ecl_kw_fskip_data(ecl_kw_type * , fortio_type *);
void          ecl_kw_fskip(fortio_type *, bool , bool );
void          ecl_kw_alloc_data(ecl_kw_type  *);
bool          ecl_kw_fread_realloc(ecl_kw_type *, fortio_type *);
ecl_kw_type * ecl_kw_fread_alloc(fortio_type * , bool , bool);
void          ecl_kw_free_data(ecl_kw_type *);
void          ecl_kw_free(ecl_kw_type *);
void          ecl_kw_set_fmt_file(ecl_kw_type *, bool);
void 	      ecl_kw_select_formatted(ecl_kw_type *);
void 	      ecl_kw_select_binary   (ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_copy (const ecl_kw_type *);
void        * ecl_kw_get_data_ref(const ecl_kw_type *);
void          ecl_kw_memcpy(ecl_kw_type *, const ecl_kw_type *);
void          ecl_kw_get_memcpy_data(const ecl_kw_type *, void *);
void          ecl_kw_set_memcpy_data(ecl_kw_type * , const void *);
void          ecl_kw_fwrite(ecl_kw_type *, fortio_type *);
void          ecl_kw_iget(const ecl_kw_type *, int , void *);
void        * ecl_kw_iget_ptr(const ecl_kw_type *, int);
int           ecl_kw_get_size(const ecl_kw_type *);
bool          ecl_kw_header_eq(const ecl_kw_type *, const char *);
bool          ecl_kw_ichar_eq(const ecl_kw_type *, int , const char *);
void        * ecl_kw_alloc_data_copy(const ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_complete(bool , bool , const char * ,  int , ecl_type_enum , const void * );
void          ecl_kw_cfwrite(const ecl_kw_type *  , FILE *);
void          ecl_kw_cfread(ecl_kw_type *  , FILE *);
bool          ecl_kw_get_endian_convert(const ecl_kw_type * );
#endif
