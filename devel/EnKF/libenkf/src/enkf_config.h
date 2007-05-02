#ifndef __ENKF_CONFIG_H__
#define __ENKF_CONFIG_H__
#include <stdbool.h>

typedef void (config_set_ensfile_ftype)  (void * , const char *);
typedef void (config_set_eclfile_ftype)  (void * , const char *);
typedef void (config_free_ftype)         (void *);


typedef struct enkf_config_struct enkf_config_type;

enkf_config_type * enkf_config_alloc(const char * , const char *, const char *);
bool               enkf_config_restart_kw(const enkf_config_type * , const char *);
void               enkf_config_add_type(enkf_config_type * , const char * , const void *,  config_set_ensfile_ftype * , config_set_eclfile_ftype * , config_free_ftype *);
const void       * enkf_config_get_node_value(const enkf_config_type * , const char * );
void               enkf_config_set_ensfile(enkf_config_type *);
void               enkf_config_set_eclfile(enkf_config_type * );
void               enkf_config_set_ens_path(enkf_config_type * , const char * );
void               enkf_config_set_ecl_path(enkf_config_type * , const char * );
void               enkf_config_set_ens_root_path(enkf_config_type * , const char * );
void               enkf_config_set_ecl_root_path(enkf_config_type * , const char * );
void               enkf_config_free(enkf_config_type * );
void               enkf_config_add_enkf_kw(const enkf_config_type *, const char *);

#endif
