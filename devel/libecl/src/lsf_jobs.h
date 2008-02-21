#ifndef __LSF_JOBS_H__
#define __LSF_JOBS_H__

typedef struct lsf_pool_struct lsf_pool_type;
typedef struct lsf_job_struct  lsf_job_type;

typedef enum  {lsf_status_null , lsf_status_submitted , lsf_status_running , lsf_status_done , lsf_status_OK , lsf_status_exit , lsf_status_complete_fail}  lsf_status_type;

lsf_pool_type *  lsf_pool_alloc(int , int , int , bool , int , const char * , const char * , const char * , const char *, const char *, const char * , const char *);
void             lsf_pool_add_job(lsf_pool_type * , int , const char *, const char * , const char *, const char *,  const char *, int);
int              lsf_pool_run_jobs(lsf_pool_type *);
void           * lsf_pool_run_jobs__(void * );
void             lsf_pool_set_fail_vector(const lsf_pool_type *  , int *);
void             lsf_pool_free(lsf_pool_type *);
int              lsf_pool_get_ijob(const lsf_pool_type * , int );
lsf_status_type  lsf_pool_iget_status(const lsf_pool_type *, int );
#endif
