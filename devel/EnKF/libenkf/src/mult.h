#ifndef __MULT_H__
#define __MULT_H__
#include <enkf_util.h>
#include <mult_config.h>
#include <stdio.h>


typedef struct mult_struct mult_type;

void             mult_truncate(mult_type * );
void             mult_transform(mult_type * );
void             mult_get_data(const mult_type * , double * );
void             mult_set_data(mult_type * , const double * );
char           * mult_alloc_ensfile(const mult_type * , const char *);
mult_type      * mult_alloc(const mult_config_type * );
void             mult_free(mult_type *);
char           * mult_alloc_ensname(const mult_type *);
char           * mult_alloc_eclname(const mult_type *);
void             mult_ecl_write(const mult_type * , const char *);
void             mult_ens_write(const mult_type * , const char *);
void             mult_ens_read(mult_type * , const char *);
void             mult_sample(mult_type *);
void             mult_truncate(mult_type *);
void             mult_TEST(void);
void             mult_stream_fwrite(const mult_type * mult , FILE * );
void             mult_stream_fread(mult_type * mult , FILE * );
void             mult_realloc_data(mult_type * mult);
void             mult_clear(mult_type * mult); 
void             mult_serialize(const mult_type * , double *, size_t *);
const double   * mult_get_output_ref(const mult_type * );
const double   * mult_get_data_ref  (const mult_type * );
void             mult_memcpy(mult_type * , const mult_type * );

MATH_OPS_HEADER(mult);
VOID_ALLOC_HEADER(mult);
VOID_FREE_HEADER(mult);
VOID_FREE_DATA_HEADER(mult);
VOID_REALLOC_DATA_HEADER(mult);
VOID_COPYC_HEADER      (mult);
VOID_ALLOC_ENSFILE_HEADER(mult);
VOID_SWAPIN_HEADER(mult)
VOID_SWAPOUT_HEADER(mult)
VOID_SERIALIZE_HEADER  (mult)
     /*VOID_TRUNCATE_HEADER(mult)*/

VOID_ECL_WRITE_HEADER (mult)
VOID_ENS_WRITE_HEADER (mult)
VOID_ENS_READ_HEADER  (mult)

VOID_FUNC_HEADER       (mult_sample   );
VOID_FUNC_HEADER       (mult_isqrt    );

#endif
