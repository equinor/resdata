#ifndef __HIST_H__
#define __HIST_H__
#include <time.h>
#include <stdbool.h>
#include <sched_file.h>
#include <ecl_sum.h>
#include <ecl_well_vars.h>
#include "date_node.h"
#include "rate_node.h"


typedef struct hist_struct hist_type;



void             hist_free(hist_type *);
hist_type * 	 hist_alloc(time_t);
hist_type * 	 hist_alloc_from_summary(const ecl_sum_type * , int , const char ** , bool);
hist_type * 	 hist_alloc_from_schedule(const sched_file_type * );
void   hist_add_date(hist_type *  , const date_node_type * );
void   hist_add_rate(hist_type *  , int , const rate_type * );

double hist_get_ORAT(const hist_type * , int , const char * ,  bool *);
double hist_get_WRAT(const hist_type * , int , const char * ,  bool *);
double hist_get_GRAT(const hist_type * , int , const char * ,  bool *);
double hist_get_THP(const hist_type * , int , const char * ,  bool *);
double hist_get_BHP(const hist_type * , int , const char * ,  bool *);
double hist_get_GOR(const hist_type * , int , const char * , bool *, bool *);
double hist_get_WCT(const hist_type * , int , const char * , bool *, bool *);

time_t 		hist_get_report_date(hist_type * , int );
double 		hist_get2(const hist_type * , int , const char * , const char * , bool *);
double 	        hist_get(const hist_type * , int , const char * , const char * );
bool   	        hist_has_well(const hist_type * , const char * );
/*well_var_type   hist_get_var_type(const hist_type *  , const char * );   */
hist_type     * hist_alloc_from_schedule(const sched_file_type *);

#endif
