#ifndef  __ECL_STATIC_KW_H__
#define  __ECL_STATIC_KW_H__

typedef struct ecl_static_kw_struct ecl_static_kw_type;

ecl_static_kw_type * ecl_static_kw_alloc();
void                 ecl_static_kw_free(ecl_static_kw_type *ecl_static_kw);

#endif
