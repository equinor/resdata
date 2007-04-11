#ifndef __MULTZ_H__
#define __MULTZ_H__

#include <enkf_util.h>
#include <enkf_state.h>

typedef struct multz_struct multz_type;

multz_type     * multz_alloc(const enkf_state_type * , const multz_config_type * );
void             multz_free(multz_type *);
char           * multz_alloc_ensname(const multz_type *);
char           * multz_alloc_eclname(const multz_type *);
void             multz_ecl_write(const multz_type * );
void             multz_ens_write(const multz_type * );
void             multz_ens_read(multz_type *);

VOID_FUNC_HEADER_CONST (multz_ecl_write  , multz_type);
VOID_FUNC_HEADER_CONST (multz_ens_write  , multz_type);
VOID_FUNC_HEADER       (multz_ens_read   , multz_type);
VOID_FUNC_HEADER       (multz_sample     , multz_type);
VOID_FUNC_HEADER       (multz_free       , multz_type);

#endif
