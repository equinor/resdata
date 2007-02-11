#ifndef __LSF_JOBS_H__
#define __LSF_JOBS_H__

typedef struct lsf_pool_struct lsf_pool_type;
typedef struct lsf_job_struct  lsf_job_type;

typedef enum   lsf_status_enum_def lsf_status_enum;

lsf_pool_type * lsf_pool_alloc(int , int , bool , const char * , const char *, const char *);
void            lsf_pool_add_job(lsf_pool_type * , const char *, const char * , const char *, const char *,  const char *, int);
int             lsf_pool_run_jobs(lsf_pool_type *);
void            lsf_pool_set_fail_vector(const lsf_pool_type *  , int *);
void            lsf_pool_free(lsf_pool_type *);
#endif
