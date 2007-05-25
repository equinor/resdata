#ifndef __MULTFLT_H__
#define __MULTFLT_H__

#include <multflt_config.h>
#include <enkf_util.h>
#include <config.h>

typedef struct multflt_struct multflt_type;

multflt_type   * multflt_alloc(const multflt_config_type * );
void             multflt_free(multflt_type *);
char           * multflt_alloc_ensname(const multflt_type *);
char           * multflt_alloc_eclname(const multflt_type *);
void             multflt_ecl_write(const multflt_type * , const char *);
void             multflt_ens_write(const multflt_type * , const char *);
void             multflt_ens_read(multflt_type * , const char *);

VOID_ECL_WRITE_HEADER  (multflt)
VOID_ENS_WRITE_HEADER  (multflt)
VOID_ENS_READ_HEADER   (multflt)
VOID_COPYC_HEADER      (multflt);
VOID_ALLOC_ENSFILE_HEADER(multflt);

VOID_FUNC_HEADER       (multflt_sample   );
VOID_FUNC_HEADER       (multflt_free     );
VOID_SERIALIZE_HEADER  (multflt_serialize);

MATH_OPS_HEADER(multflt);
VOID_ALLOC_HEADER(multflt);

#endif
