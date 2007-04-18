#ifndef __ENKF_ECL_KW_CONFIG_H__
#define __ENKF_ECL_KW_CONFIG_H__
#include <enkf_util.h>


typedef struct enkf_ecl_kw_config_struct enkf_ecl_kw_config_type;

struct enkf_ecl_kw_config_struct {
  CONFIG_STD_FIELDS;
  char *ens_file;
};


#endif
