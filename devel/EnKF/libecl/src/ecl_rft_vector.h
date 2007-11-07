#ifndef __ECL_RFT_VECTOR_H__
#define __ECL_RFT_VECTOR_H__
#include <stdbool.h>

typedef struct ecl_rft_vector_struct ecl_rft_vector_type;


ecl_rft_vector_type *  ecl_rft_vector_alloc(const char * , bool );
void                   ecl_rft_vector_free(ecl_rft_vector_type * );
void                   ecl_rft_vector_block(const ecl_rft_vector_type *  , const char * , int , const double * , int * , int * , int *);
void                   ecl_rft_vector_fprintf_rft_obs(const ecl_rft_vector_type  * , const char * , const char *, const char * , double);
char **                ecl_rft_vector_alloc_well_list(const ecl_rft_vector_type *  , int *);

#endif
