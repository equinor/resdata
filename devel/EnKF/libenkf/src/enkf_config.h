#ifndef __ENKF_CONFIG_H__
#define __ENKF_CONFIG_H__

typedef void (config_set_ensfile_ftype)  (void * , const char *);
typedef void (config_set_eclfile_ftype)  (void * , const char *);
typedef void (config_free_ftype)         (void *);


typedef struct enkf_config_struct enkf_config_type;

bool      enkf_config_restart_kw(const enkf_config_type * , const char *);

#endif
