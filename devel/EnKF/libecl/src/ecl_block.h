#ifndef __ECL_BLOCK_H__
#define __ECL_BLOCK_H__

#include <stdbool.h>
#include <ecl_kw.h>
#include <fortio.h>

typedef struct ecl_block_struct ecl_block_type;

#define SHARED    0
#define OWNED_REF 1
#define COPY      2



bool             ecl_block_fseek(int , bool , bool , fortio_type * );
ecl_kw_type    * ecl_block_get_first_kw(const ecl_block_type * );
ecl_kw_type    * ecl_block_get_next_kw(const ecl_block_type *  , const ecl_kw_type * );
void 		 ecl_block_fread_kwlist(ecl_block_type *, fortio_type *, int , const char **);
void             ecl_block_fread(ecl_block_type *, fortio_type * , bool *); 
ecl_kw_type    * ecl_block_get_kw(const ecl_block_type *, const char *);
void           * ecl_block_get_data_ref(const ecl_block_type *, const char *);
ecl_block_type * ecl_block_alloc(int , int , bool , bool);
ecl_block_type * ecl_block_alloc_copy(const ecl_block_type *);
void             ecl_block_free(ecl_block_type *);
bool             ecl_block_add_kw(ecl_block_type * , const ecl_kw_type *, int );
bool             ecl_block_add_kw_copy(ecl_block_type * , const ecl_kw_type *);
void             ecl_block_fwrite(ecl_block_type * , fortio_type *);
void             ecl_block_set_fmt_file(ecl_block_type *, bool);
void 	         ecl_block_select_formatted(ecl_block_type *);
void 	         ecl_block_select_binary   (ecl_block_type *);
int              ecl_block_get_block(const ecl_block_type *);
void             ecl_block_printf_kwlist(const ecl_block_type *);
ecl_kw_type    * ecl_block_detach_kw(ecl_block_type * , const char *);
void             ecl_block_free_kw(ecl_block_type *, const char *);
#endif
