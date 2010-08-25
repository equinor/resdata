#ifndef __ECL_RFT_FILE_H__
#define __ECL_RFT_FILE_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <ecl_rft_node.h>
#include <stringlist.h>

typedef struct ecl_rft_file_struct ecl_rft_file_type;

char                 * ecl_rft_file_alloc_case_filename(const char * case_input );
const char           * ecl_rft_file_get_filename( const ecl_rft_file_type * rft_file );
ecl_rft_file_type    * ecl_rft_file_alloc_case(const char * case_input ); 
bool                   ecl_rft_file_case_has_rft(const char * case_input ); 
bool                   ecl_rft_file_case_has_rft( const char * case_input );
ecl_rft_file_type    * ecl_rft_file_alloc(const char * );
void                   ecl_rft_file_free(ecl_rft_file_type * );
void                   ecl_rft_file_block(const ecl_rft_file_type *  , double , const char * , int , const double * , int * , int * , int *);
void                   ecl_rft_file_fprintf_rft_obs(const ecl_rft_file_type  * , double , const char * , const char *, const char * , double);
ecl_rft_node_type    * ecl_rft_file_get_node(const ecl_rft_file_type * , const char * );
void                   ecl_rft_file_summarize(const ecl_rft_file_type * , bool );
void                   ecl_rft_file_xml_summary( const ecl_rft_file_type * rft_file );

const ecl_rft_node_type * ecl_rft_file_get_well_time_rft( const ecl_rft_file_type * rft_file , const char * well , time_t recording_time);

int                       ecl_rft_file_get_size( const ecl_rft_file_type * rft_file);
const ecl_rft_node_type * ecl_rft_file_iget_node( const ecl_rft_file_type * rft_file , int index);
const ecl_rft_node_type * ecl_rft_file_iget_well_rft( const ecl_rft_file_type * rft_file , const char * well, int index);
bool 			  ecl_rft_file_has_well( const ecl_rft_file_type * rft_file , const char * well);
int  			  ecl_rft_file_get_well_occurences( const ecl_rft_file_type * rft_file , const char * well);
stringlist_type         * ecl_rft_file_alloc_well_list(const ecl_rft_file_type * rft_file );
int                       ecl_rft_file_get_num_wells( const ecl_rft_file_type * rft_file );
void                      ecl_rft_file_free__( void * arg);

#ifdef __cplusplus
}
#endif
#endif
