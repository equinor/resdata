#ifndef __SCHED_KW_DATES__
#define __SCHED_KW_DATES__
#include <hash.h>
#include <stdbool.h>
#include "date_node.h"

typedef struct sched_kw_dates_struct sched_kw_dates_type;

void                  sched_kw_dates_fprintf(const sched_kw_dates_type *, FILE *, int , time_t , bool *);
void                  sched_kw_dates_add_line(sched_kw_dates_type *, const char *, const hash_type * , bool);
sched_kw_dates_type * sched_kw_dates_alloc(int *);
void                  sched_kw_dates_free(sched_kw_dates_type * );
sched_kw_dates_type * sched_kw_dates_fread_alloc(int *  , int , time_t , FILE * , bool *);
void                  sched_kw_dates_fwrite(const sched_kw_dates_type * , FILE *);
void                  sched_kw_dates_iterate_current(const sched_kw_dates_type * , date_node_type **);
#endif
