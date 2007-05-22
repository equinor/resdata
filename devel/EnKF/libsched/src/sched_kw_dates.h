#ifndef __SCHED_KW_DATES__
#define __SCHED_KW_DATES__
#include <hash.h>

typedef struct sched_kw_dates_struct sched_kw_dates_type;
typedef struct date_node_struct      date_node_type;

int                   date_node_get_date_nr(const date_node_type * );
void                  date_node_fprintf_rate_date(const date_node_type * , const char * , const char *);

void                  sched_kw_dates_fprintf(const sched_kw_dates_type *, FILE *, int , time_t , bool *);
void                  sched_kw_dates_add_line(sched_kw_dates_type *, const char *, const hash_type *);
sched_kw_dates_type * sched_kw_dates_alloc(int *);
void                  sched_kw_dates_free(sched_kw_dates_type * );
sched_kw_dates_type * sched_kw_dates_fread_alloc(int *  , int , time_t , FILE * , bool *);
void                  sched_kw_dates_fwrite(const sched_kw_dates_type * , FILE *);
void                  sched_kw_dates_iterate_current(const sched_kw_dates_type * , date_node_type **);
#endif
