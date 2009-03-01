#ifndef __ECL_SUM_H__
#define __ECL_SUM_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <hash.h>

typedef struct ecl_sum_struct       ecl_sum_type;



void             ecl_sum_fread_realloc_data(ecl_sum_type *, int , const char ** , bool);
void             ecl_sum_free_data(ecl_sum_type * );
void             ecl_sum_free(ecl_sum_type * );
ecl_sum_type   * ecl_sum_fread_alloc(const char * , int , const char **, bool);
ecl_sum_type   * ecl_sum_fread_alloc_case(const char *  , bool );

/* Accessor functions : */
double            ecl_sum_get_well_var(const ecl_sum_type * ecl_sum , int ministep , const char * well , const char *var);
int               ecl_sum_get_well_var_index(const ecl_sum_type * ecl_sum , const char * well , const char *var);
bool              ecl_sum_has_well_var(const ecl_sum_type * ecl_sum , const char * well , const char *var);

double            ecl_sum_get_group_var(const ecl_sum_type * ecl_sum , int ministep , const char * group , const char *var);
int               ecl_sum_get_group_var_index(const ecl_sum_type * ecl_sum , const char * group , const char *var);
bool              ecl_sum_has_group_var(const ecl_sum_type * ecl_sum , const char * group , const char *var);

double            ecl_sum_get_field_var(const ecl_sum_type * ecl_sum , int ministep , const char *var);
int               ecl_sum_get_field_var_index(const ecl_sum_type * ecl_sum , const char *var);
bool              ecl_sum_has_field_var(const ecl_sum_type * ecl_sum , const char *var);

double            ecl_sum_get_block_var(const ecl_sum_type * ecl_sum , int ministep , const char * block_var , int block_nr);
int               ecl_sum_get_block_var_index(const ecl_sum_type * ecl_sum , const char * block_var , int block_nr);
bool              ecl_sum_has_block_var(const ecl_sum_type * ecl_sum , const char * block_var , int block_nr);

double            ecl_sum_get_region_var(const ecl_sum_type * ecl_sum , int ministep , int region_nr , const char *var);
int               ecl_sum_get_region_var_index(const ecl_sum_type * ecl_sum , int region_nr , const char *var);
bool              ecl_sum_has_region_var(const ecl_sum_type * ecl_sum , int region_nr , const char *var);

double            ecl_sum_get_misc_var(const ecl_sum_type * ecl_sum , int ministep , const char *var);
int               ecl_sum_get_misc_var_index(const ecl_sum_type * ecl_sum , const char *var);
bool              ecl_sum_has_misc_var(const ecl_sum_type * ecl_sum , const char *var);

double            ecl_sum_get_well_completion_var(const ecl_sum_type * ecl_sum , int ministep , const char * well , const char *var, int cell_nr);
int               ecl_sum_get_well_completion_var_index(const ecl_sum_type * ecl_sum , const char * well , const char *var, int cell_nr);
bool              ecl_sum_has_well_completion_var(const ecl_sum_type * ecl_sum , const char * well , const char *var, int cell_nr);

double            ecl_sum_get_general_var(const ecl_sum_type * ecl_sum , int ministep , const char * lookup_kw);
int               ecl_sum_get_general_var_index(const ecl_sum_type * ecl_sum , const char * lookup_kw);
bool              ecl_sum_has_general_var(const ecl_sum_type * ecl_sum , const char * lookup_kw);
/***************/
void             ecl_sum_fprintf(const ecl_sum_type * , FILE * , int , const char **  );

/* Time related functions */
bool   ecl_sum_has_report_step(const ecl_sum_type * ecl_sum , int report_step );
bool   ecl_sum_has_ministep(const ecl_sum_type * ecl_sum , int ministep );
void   ecl_sum_get_ministep_range(const ecl_sum_type * ecl_sum , int * ministep1, int * ministep2);
void   ecl_sum_report2ministep_range(const ecl_sum_type * ecl_sum , int report_step , int * ministep1 , int * ministep2 );
time_t ecl_sum_get_sim_time( const ecl_sum_type * ecl_sum , int ministep );
double ecl_sum_get_sim_days( const ecl_sum_type * ecl_sum , int ministep );


const char * ecl_sum_get_simulation_case(const ecl_sum_type * );
/*****************************************************************/
/* Legacy shit: */
int           ecl_sum_get_num_wells(const ecl_sum_type *);
const char ** ecl_sum_get_well_names(const ecl_sum_type * );

  


#ifdef __cplusplus
}
#endif
#endif
