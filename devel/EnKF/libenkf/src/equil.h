#ifndef __EQUIL_H__
#define __EQUIL_H__

#include <enkf_state.h>
#include <enkf_util.h>

typedef struct equil_struct equil_type;

equil_type     * equil_alloc(const enkf_state_type * , const equil_config_type * );
void             equil_free(equil_type *);
char           * equil_alloc_ensname(const equil_type *);
char           * equil_alloc_eclname(const equil_type *);
void             equil_ecl_write(const equil_type * );
void             equil_ens_write(const equil_type * );
void             equil_ens_read(equil_type *);

VOID_FUNC_HEADER_CONST (equil_ecl_write);
VOID_FUNC_HEADER_CONST (equil_ens_write);
VOID_FUNC_HEADER       (equil_ens_read );
VOID_FUNC_HEADER       (equil_sample   );
VOID_FUNC_HEADER       (equil_free     );

MATH_OPS_HEADER(equil);

#endif
