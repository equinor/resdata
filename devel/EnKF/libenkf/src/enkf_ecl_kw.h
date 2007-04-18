#ifndef  __ENKF_ECL_KW_H__
#define  __ENKF_ECL_KW_H__
#include <enkf_util.h>
typedef struct enkf_ecl_kw_struct enkf_ecl_kw_type;

enkf_ecl_kw_type * enkf_ecl_kw_alloc();
void               enkf_ecl_kw_free(enkf_ecl_kw_type *enkf_ecl_kw);

MATH_OPS_HEADER(enkf_ecl_kw);


#endif
