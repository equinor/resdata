#ifndef __ECL_KW_H__
#define __ECL_KW_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fortio.h>
#include <ecl_util.h>




typedef struct ecl_kw_struct      ecl_kw_type;


ecl_type_enum ecl_kw_get_type(const ecl_kw_type *);
const char  * ecl_kw_get_str_type_ref(const ecl_kw_type *);
/*const char  * ecl_kw_str_type(ecl_type_enum );*/
void          ecl_kw_fread_data(ecl_kw_type *, fortio_type *);
const char  * ecl_kw_get_header_ref(const ecl_kw_type *);
char        * ecl_kw_alloc_strip_header(const ecl_kw_type * );
ecl_kw_type * ecl_kw_alloc_empty(bool , bool);
void          ecl_kw_rewind(const ecl_kw_type * , fortio_type *);
void          ecl_kw_copy_data(const ecl_kw_type * , void *);
bool          ecl_kw_fread_header(ecl_kw_type *, fortio_type *);
void          ecl_kw_set_header_name(ecl_kw_type * , const char * );
void          ecl_kw_set_header(ecl_kw_type  * , const char * , int , const char *);
void          ecl_kw_set_header_alloc(ecl_kw_type  * , const char * , int , const char *);
bool          ecl_kw_fseek_kw(const char * , bool , bool , bool , fortio_type *);
bool          ecl_kw_fseek_last_kw(const char * , bool , bool  , fortio_type *);
void          ecl_kw_fskip_data(ecl_kw_type * , fortio_type *);
void          ecl_kw_fskip(fortio_type *, bool);
void          ecl_kw_alloc_data(ecl_kw_type  *);
bool          ecl_kw_fread_realloc(ecl_kw_type *, fortio_type *);
ecl_kw_type * ecl_kw_fscanf_alloc_grdecl_data(FILE * , int , ecl_type_enum , bool );
void          ecl_kw_fprintf_grdecl(ecl_kw_type *  , FILE * );
ecl_kw_type * ecl_kw_fread_alloc(fortio_type * , bool);
ecl_kw_type * ecl_kw_fscanf_alloc_parameter(FILE * , int , bool );
void          ecl_kw_free_data(ecl_kw_type *);
void          ecl_kw_free(ecl_kw_type *);
void          ecl_kw_free__(void *);
void          ecl_kw_set_fmt_file(ecl_kw_type *, bool);
void 	      ecl_kw_select_formatted(ecl_kw_type *);
void 	      ecl_kw_select_binary   (ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_copy (const ecl_kw_type *);
const void  * ecl_kw_copyc__(const void *);
void        * ecl_kw_get_data_ref(const ecl_kw_type *);
void        * ecl_kw_get_safe_data_ref(const ecl_kw_type * , ecl_type_enum );
void          ecl_kw_memcpy(ecl_kw_type *, const ecl_kw_type *);
void          ecl_kw_get_memcpy_data(const ecl_kw_type *, void *);
void          ecl_kw_set_memcpy_data(ecl_kw_type * , const void *);
void          ecl_kw_fwrite(const ecl_kw_type *, fortio_type *);
void          ecl_kw_iget(const ecl_kw_type *, int , void *);
void        * ecl_kw_iget_ptr(const ecl_kw_type *, int);
int           ecl_kw_get_size(const ecl_kw_type *);
bool          ecl_kw_header_eq(const ecl_kw_type *, const char *);
bool          ecl_kw_ichar_eq(const ecl_kw_type *, int , const char *);
void        * ecl_kw_alloc_data_copy(const ecl_kw_type *);
ecl_kw_type * ecl_kw_alloc_complete(bool , bool , const char * ,  int , ecl_type_enum , const void * );
ecl_kw_type * ecl_kw_alloc_complete_shared(bool , bool , const char * ,  int , ecl_type_enum , void * );
void          ecl_kw_cfwrite(const ecl_kw_type *  , FILE *);
void          ecl_kw_cfread(ecl_kw_type *  , FILE *);
bool          ecl_kw_get_endian_convert(const ecl_kw_type * );
void          ecl_kw_fwrite_param(const char * , bool  , bool , const char * ,  ecl_type_enum , int , void * );
void          ecl_kw_fwrite_param_fortio(fortio_type *, bool  , bool , const char * ,  ecl_type_enum , int , void * );
void          ecl_kw_summarize(const ecl_kw_type * ecl_kw);
void          ecl_kw_fread_double_param(const char * , bool , bool , double *);
void          ecl_kw_get_data_as_double(const ecl_kw_type *, double *);

bool ecl_kw_is_grdecl_file(FILE * );
bool ecl_kw_is_kw_file(FILE * , bool , bool );
void ecl_kw_inplace_sub(ecl_kw_type * , const ecl_kw_type * );
void ecl_kw_inplace_mul(ecl_kw_type * , const ecl_kw_type * );
void ecl_kw_inplace_add(ecl_kw_type * , const ecl_kw_type * );
void ecl_kw_inplace_div(ecl_kw_type * , const ecl_kw_type * );

void ecl_kw_inplace_inv(ecl_kw_type * my_kw);
void ecl_kw_scalar_init(ecl_kw_type * , double );
void ecl_kw_scale(ecl_kw_type * , double );
void ecl_kw_shift(ecl_kw_type * , double );
void ecl_kw_merge(ecl_kw_type * , const ecl_kw_type * , const ecl_box_type * );
void ecl_kw_element_sum(const ecl_kw_type * , void * );
void ecl_kw_max_min(const ecl_kw_type * , void * , void *);
#endif
