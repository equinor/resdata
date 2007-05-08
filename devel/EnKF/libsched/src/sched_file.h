#ifndef __SCHED_FILE_H__
#define __SCHED_FILE_H__
#include <time.h>

typedef struct sched_file_struct sched_file_type;

void 		  sched_file_init_conn_factor(sched_file_type * , const char *  , bool , const int * );
void 		  sched_file_set_conn_factor(sched_file_type *  , const float * , const float * , const int *,  const int * );
sched_file_type * sched_file_alloc(void);
void              sched_file_fprintf(const sched_file_type * , int , time_t , const char * );
sched_file_type * sched_file_fread_alloc(const char * , int , time_t );
void              sched_file_fwrite(const sched_file_type * , const char *);
void              sched_file_parse(sched_file_type * , const char * );
void              sched_file_free(sched_file_type *);
void              sched_file_fprintf_rates(const sched_file_type * , const char * , const char *);
#endif
