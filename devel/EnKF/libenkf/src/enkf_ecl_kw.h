#ifndef  __ENKF_ECL_KW_H__
#define  __ENKF_ECL_KW_H__
#include <enkf_util.h>
#include <enkf_state.h>

typedef struct     enkf_ecl_kw_struct enkf_ecl_kw_type;

void               enkf_ecl_kw_get_data(enkf_ecl_kw_type * , const ecl_kw_type *);
enkf_ecl_kw_type * enkf_ecl_kw_alloc(const enkf_state_type * , const char * , int , const char * , enkf_var_type);
void               enkf_ecl_kw_free(enkf_ecl_kw_type *);

MATH_OPS_HEADER(enkf_ecl_kw);


#endif
