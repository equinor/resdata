#ifndef __EQUIL_H__
#define __EQUIL_H__

#include <config.h>
#include <equil_config.h>
#include <enkf_util.h>

typedef struct equil_struct equil_type;

equil_type     * equil_copyc(const equil_type * );
equil_type     * equil_alloc(const equil_config_type * );
void             equil_free(equil_type *);
char           * equil_alloc_ensname(const equil_type *);
char           * equil_alloc_eclname(const equil_type *);
void             equil_ecl_write(const equil_type * , const char *);
void             equil_ens_write(const equil_type * , const char *);
void             equil_ens_read(equil_type * , const char *);

VOID_SWAPOUT_HEADER(equil);
VOID_SWAPIN_HEADER(equil);
VOID_ALLOC_ENSFILE_HEADER(equil);
VOID_ECL_WRITE_HEADER  (equil)
VOID_ENS_WRITE_HEADER  (equil)
VOID_ENS_READ_HEADER   (equil)
VOID_FUNC_HEADER       (equil_sample   );
VOID_FUNC_HEADER       (equil_free     );
VOID_COPYC_HEADER      (equil);
MATH_OPS_HEADER(equil);
VOID_ALLOC_HEADER(equil);

#endif
