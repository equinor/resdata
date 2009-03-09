#ifndef __ECL_RFT_NODE_H__
#define __ECL_RFT_NODE_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <ecl_file.h>
#include <stdbool.h>

typedef struct ecl_rft_node_struct ecl_rft_node_type;

void                ecl_rft_node_fprintf_rft_obs(const ecl_rft_node_type * , double , const char * , const char * , double );
ecl_rft_node_type * ecl_rft_node_alloc(const ecl_file_type * );
const char        * ecl_rft_node_get_well_name(const ecl_rft_node_type * );
void                ecl_rft_node_free(ecl_rft_node_type * );
void                ecl_rft_node_free__(void * );
void                ecl_rft_node_block2(const ecl_rft_node_type * , int  , const double * , const double * , int * , int * , int *);
void                ecl_rft_node_block(const ecl_rft_node_type *  , double , int , const double * , int * , int *  , int *);
time_t    	    ecl_rft_node_get_recording_time(const ecl_rft_node_type * );
int                 ecl_rft_node_get_size(const ecl_rft_node_type * );
void                ecl_rft_node_summarize(const ecl_rft_node_type * , bool );

void ecl_rft_node_export_DEPTH(const ecl_rft_node_type * , const char * );

#ifdef __cplusplus
}
#endif
#endif

