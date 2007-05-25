#ifndef __MULTZ_H__
#define __MULTZ_H__
#include <enkf_util.h>
#include <multz_config.h>


typedef struct multz_struct multz_type;

const     char * multz_alloc_ensfile(const multz_type * , const char *);
multz_type     * multz_alloc(const multz_config_type * );
void             multz_free(multz_type *);
char           * multz_alloc_ensname(const multz_type *);
char           * multz_alloc_eclname(const multz_type *);
void             multz_ecl_write(const multz_type * , const char *);
void             multz_ens_write(const multz_type * , const char *);
void             multz_ens_read(multz_type * , const char *);



MATH_OPS_HEADER(multz);
VOID_ALLOC_HEADER(multz);
VOID_FREE_HEADER(multz);
VOID_FREE_DATA_HEADER(multz);
VOID_REALLOC_DATA_HEADER(multz);
VOID_COPYC_HEADER      (multz);
VOID_ALLOC_ENSFILE_HEADER(multz);


VOID_ECL_WRITE_HEADER (multz)
VOID_ENS_WRITE_HEADER (multz)
VOID_ENS_READ_HEADER  (multz)


VOID_FUNC_HEADER       (multz_sample   );
VOID_FUNC_HEADER       (multz_isqrt    );
VOID_SERIALIZE_HEADER  (multz_serialize);
#endif
