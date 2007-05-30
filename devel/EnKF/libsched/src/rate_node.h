#ifndef __RATE_NODE_H__
#define __RATE_NODE_H__

typedef struct rate_struct rate_type;

void         rate_sched_fprintf(const rate_type *  , FILE *);
rate_type *  rate_alloc(int , const char **);
void         rate_sched_fwrite(const rate_type *,  FILE *);
rate_type *  rate_sched_fread_alloc(FILE *);
void         rate_sched_fprintf_rates(const rate_type * , FILE *);
void         rate_free(rate_type *);
void         rate_free__(void *__rate);
const char * rate_get_well_ref(const rate_type * );
const void * rate_copyc__(const void *);
const char * rate_node_get_well_ref(const rate_type * );

double 	   rate_get_ORAT(const rate_type * );
double 	   rate_get_GRAT(const rate_type * );
double 	   rate_get_WRAT(const rate_type * );
double 	   rate_get_GOR(const rate_type * , bool *);
double 	   rate_get_WCT(const rate_type * , bool *);


#endif

