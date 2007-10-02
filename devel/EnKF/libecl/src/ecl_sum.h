#ifndef __ECL_SUM_H__
#define __ECL_SUM_H__

#include <stdbool.h>
#include <hash.h>

typedef struct ecl_sum_struct ecl_sum_type;

void           ecl_sum_fread_alloc_data(ecl_sum_type * , int , const char ** , bool);
ecl_sum_type * ecl_sum_fread_alloc(const char * , int , const char **, bool  , bool );

int            ecl_sum_get_Nwells(const ecl_sum_type *);
void           ecl_sum_copy_well_names(const ecl_sum_type * , char **);
void           ecl_sum_init_save(ecl_sum_type * , const char * , int , bool);
/*
  ecl_sum_type * ecl_sum_load_unified(const char * , const char * , int , bool);
  ecl_sum_type * ecl_sum_load_multiple(const char * , int , const char ** , int , bool);
  ecl_sum_type * ecl_sum_load_single(const char * , const char * , int , bool );
*/
bool           ecl_sum_has_kw(const ecl_sum_type * , const char * , const char *);
int            ecl_sum_get_index(const ecl_sum_type * , const char * , const char *);
double         ecl_sum_iget1(const ecl_sum_type *, int , const char *, const char * ,  int *);
double         ecl_sum_iget2(const ecl_sum_type *, int , int);
void           ecl_sum_free_data(ecl_sum_type * );
void           ecl_sum_free(ecl_sum_type *);
int            ecl_sum_get_size(const ecl_sum_type *);
void           ecl_sum_set_fmt_mode(ecl_sum_type *, int );
ecl_sum_type * ecl_sum_alloc_new(const char * , int, int, int, int , bool , bool , bool );
void           ecl_sum_save(const ecl_sum_type * );
void           ecl_sum_set_header_data(ecl_sum_type * , const char * , void *);
char        ** ecl_sum_alloc_well_names_copy(const ecl_sum_type *);
char        ** ecl_sum_alloc_filelist(const char *, const char *, bool , int *);
bool           ecl_sum_get_report_mode(const ecl_sum_type * );
time_t         ecl_sum_get_start_time(const ecl_sum_type * );
time_t         ecl_sum_get_sim_time(const ecl_sum_type * , int );
int            ecl_sum_get_report_size(const ecl_sum_type * , int * , int * );

double         ecl_sum_eval_well_misfit(const ecl_sum_type * , const char * , int , const char ** , const double * );
double         ecl_sum_eval_misfit(const ecl_sum_type * , int , const char ** , int , const char ** , const double * ,  const double * );
#endif
