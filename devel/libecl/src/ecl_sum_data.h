#ifndef __ECL_SUM_DATA_H__
#define __ECL_SUM_DATA_H__


#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <time.h>

typedef struct ecl_sum_data_struct ecl_sum_data_type ; 


time_t                   ecl_sum_data_get_sim_start ( const ecl_sum_data_type * data ); 
time_t                   ecl_sum_data_get_sim_end   ( const ecl_sum_data_type * data ); 
double                   ecl_sum_data_get_sim_length( const ecl_sum_data_type * data ); 
void                     ecl_sum_data_summarize(const ecl_sum_data_type * data , FILE * stream);
bool        	         ecl_sum_data_has_ministep(const ecl_sum_data_type *  , int );
double      	         ecl_sum_data_get(const ecl_sum_data_type * , int , int );
void        	         ecl_sum_data_get_ministep_range(const ecl_sum_data_type *  , int * , int *);
double      	         ecl_sum_data_get_sim_days( const ecl_sum_data_type *  , int );
time_t      	         ecl_sum_data_get_sim_time( const ecl_sum_data_type *  , int );
bool        	         ecl_sum_data_has_report_step(const ecl_sum_data_type *  , int );
void        	         ecl_sum_data_report2ministep_range(const ecl_sum_data_type *  , int  , int *  , int * );
ecl_sum_data_type      * ecl_sum_data_fread_alloc(const ecl_smspec_type *  , int  , const char **);
void                     ecl_sum_data_free( ecl_sum_data_type * );
int                      ecl_sum_data_get_last_report_step( const ecl_sum_data_type * data );
int                      ecl_sum_data_get_first_report_step( const ecl_sum_data_type * data );
int 			 ecl_sum_data_get_first_ministep( const ecl_sum_data_type * data );
int 			 ecl_sum_data_get_last_ministep( const ecl_sum_data_type * data );

time_t                   ecl_sum_data_get_sim_end   ( const ecl_sum_data_type * data );
int                      ecl_sum_data_get_ministep_from_sim_days( const ecl_sum_data_type * data , double sim_days);
int                      ecl_sum_data_get_ministep_from_sim_time( const ecl_sum_data_type * data , time_t sim_time);

double                   ecl_sum_data_get_from_sim_time( const ecl_sum_data_type * data , time_t sim_time , int params_index);
double                   ecl_sum_data_get_from_sim_days( const ecl_sum_data_type * data , double sim_days , int params_index);


#ifdef __cplusplus
}
#endif
#endif
