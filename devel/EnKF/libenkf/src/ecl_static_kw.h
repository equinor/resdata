#ifndef  __ECL_STATIC_KW_H__
#define  __ECL_STATIC_KW_H__
#include <enkf_state.h>
typedef struct ecl_static_kw_struct ecl_static_kw_type;


ecl_static_kw_type * ecl_static_kw_alloc(const enkf_state_type *);
void                 ecl_static_kw_free(ecl_static_kw_type *ecl_static_kw);

#endif
