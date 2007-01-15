#ifndef __EXT_JOB_H__
#define __EXT_JOB_H__
#include <stdbool.h>

typedef struct ext_job_struct      ext_job_type;
typedef enum   ext_status_enum_def ext_status_enum;
typedef enum   ext_action_enum_def ext_action_enum;


enum ext_action_enum_def {ext_action_null , ext_action_submit    , ext_action_watch   , ext_action_kill        , ext_action_restart , ext_action_return};
enum ext_status_enum_def {ext_status_null , ext_status_submitted , ext_status_running , ext_status_complete_OK , ext_status_complete_kill , ext_status_complete_fail};

void              ext_job_free(ext_job_type *);
ext_job_type    * ext_job_alloc(const char * ,const char *, const char *, const char * , const char *, const char * , int , int , bool);
ext_status_enum   ext_job_get_status(ext_job_type * );
void              ext_job_set_action(ext_job_type * , ext_action_enum );
void              ext_job_run_pool(int , ext_job_type **, int , int , const char *, bool);
#endif
