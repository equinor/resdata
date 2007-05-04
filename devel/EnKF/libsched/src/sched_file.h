#ifndef __SCHED_FILE_H__
#define __SCHED_FILE_H__

typedef struct sched_file_struct sched_file_type;

void 		  sched_file_init_conn_factor(sched_file_type * , const char *  , bool , const int * );
void 		  sched_file_set_conn_factor(sched_file_type *  , const float * , const int *,  const int * );
sched_file_type * sched_file_alloc(void);
void              sched_file_fprintf(const sched_file_type * , int , time_t , const char * );

#endif
