#ifndef __ECL_RFT_VECTOR_H__
#define __ECL_RFT_VECTOR_H__
#include <stdbool.h>

typedef struct ecl_rft_vector_struct ecl_rft_vector_type;


ecl_rft_vector_type *  ecl_rft_vector_alloc(const char * , bool );
void                   ecl_rft_vector_free(ecl_rft_vector_type * );
void                   ecl_rft_vector_block(const ecl_rft_vector_type *  , const char * , int , const double * , int * , int * , int *);

#endif
