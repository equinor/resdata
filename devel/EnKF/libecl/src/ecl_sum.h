#ifndef __ECL_SUM_H__
#define __ECL_SUM_H__

#include <stdbool.h>

typedef struct ecl_sum_struct ecl_sum_type;


ecl_sum_type * ecl_sum_load_unified(const char * , const char * , int , bool);
ecl_sum_type * ecl_sum_load_multiple(const char * , int , const char ** , int , bool);
int            ecl_sum_iget1(const ecl_sum_type *, int , const char *, const char * , void *);
void           ecl_sum_iget2(const ecl_sum_type *, int , int , void *);
void           ecl_sum_free(ecl_sum_type *);
int            ecl_sum_get_size(const ecl_sum_type *);

#endif
