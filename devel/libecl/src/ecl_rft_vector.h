#ifndef __ECL_RFT_VECTOR_H__
#define __ECL_RFT_VECTOR_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <ecl_rft_node.h>

typedef struct ecl_rft_vector_struct ecl_rft_vector_type;


ecl_rft_vector_type *  ecl_rft_vector_alloc(const char * , bool );
void                   ecl_rft_vector_free(ecl_rft_vector_type * );
void                   ecl_rft_vector_block(const ecl_rft_vector_type *  , double , const char * , int , const double * , int * , int * , int *);
void                   ecl_rft_vector_fprintf_rft_obs(const ecl_rft_vector_type  * , double , const char * , const char *, const char * , double);
char **                ecl_rft_vector_alloc_well_list(const ecl_rft_vector_type *  , int *);
ecl_rft_node_type    * ecl_rft_vector_get_node(const ecl_rft_vector_type * , const char * );
void                   ecl_rft_vector_summarize(const ecl_rft_vector_type * , bool );

#ifdef __cplusplus
}
#endif
#endif
