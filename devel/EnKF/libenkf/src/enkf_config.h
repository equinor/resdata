#ifndef __ENKF_CONFIG_H__
#define __ENKF_CONFIG_H__
#include <stdbool.h>

#define LOG_INPUT    1
#define LOG_ENKF     2 
#define LOG_OUTPUT   4


/*****************************************************************/


typedef void (config_free_ftype)         (void *);
typedef int  (config_get_size_ftype)     (const void *);


typedef struct enkf_config_struct enkf_config_type;

int                enkf_config_get_eclpath_depth(const enkf_config_type * );
int                enkf_config_get_enspath_depth(const enkf_config_type * );
enkf_config_type * enkf_config_alloc(int , int);
bool               enkf_config_restart_kw(const enkf_config_type * , const char *);
bool               enkf_config_has_key(const enkf_config_type * , const char *);
void               enkf_config_add_type(enkf_config_type * , const char * , const void * ,  config_free_ftype *, config_get_size_ftype *);
const void       * enkf_config_get_ref(const enkf_config_type * , const char * );
void               enkf_config_free(enkf_config_type * );
void               enkf_config_add_restart_type(const enkf_config_type *, const char *);

#endif
