#ifndef  __ENKF_ECL_KW_H__
#define  __ENKF_ECL_KW_H__
#include <ecl_kw.h>
#include <enkf_util.h>
#include <enkf_state.h>

typedef struct     enkf_ecl_kw_struct enkf_ecl_kw_type;

ecl_kw_type      * enkf_ecl_kw_alloc_ecl_kw(const enkf_ecl_kw_type * , bool  , bool );
void               enkf_ecl_kw_get_data(enkf_ecl_kw_type * , const ecl_kw_type *);
enkf_ecl_kw_type * enkf_ecl_kw_alloc(const enkf_state_type * , const char * , int , const char *);
void               enkf_ecl_kw_free(enkf_ecl_kw_type *);
enkf_ecl_kw_type * enkf_kw_copyc(const enkf_ecl_kw_type * );

MATH_OPS_HEADER(enkf_ecl_kw);


#endif
