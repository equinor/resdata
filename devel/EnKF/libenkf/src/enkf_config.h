#ifndef __ENKF_CONFIG_H__
#define __ENKF_CONFIG_H__


typedef struct enkf_config_struct enkf_config_type;

bool      enkf_config_restart_kw(const enkf_config_type * , const char *);

#endif
