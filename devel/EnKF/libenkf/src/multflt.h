#ifndef __MULTFLT_H__
#define __MULTFLT_H__

#include <enkf_util.h>

typedef struct multflt_struct multflt_type;

multflt_type     * multflt_alloc(const mem_config_type * , const multflt_config_type * );
void             multflt_free(multflt_type *);
char           * multflt_alloc_ensname(const multflt_type *);
char           * multflt_alloc_eclname(const multflt_type *);
void             multflt_ecl_write(const multflt_type * );
void             multflt_ens_write(const multflt_type * );
void             multflt_ens_read(multflt_type *);

VOID_FUNC_HEADER_CONST (multflt_ecl_write  , multflt_type);
VOID_FUNC_HEADER_CONST (multflt_ens_write  , multflt_type);
VOID_FUNC_HEADER       (multflt_ens_read   , multflt_type);
VOID_FUNC_HEADER       (multflt_sample     , multflt_type);

#endif
