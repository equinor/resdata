#ifndef __ENKF_ECL_KW_CONFIG_H__
#define __ENKF_ECL_KW_CONFIG_H__
#include <enkf_util.h>


typedef struct enkf_ecl_kw_config_struct enkf_ecl_kw_config_type;

struct enkf_ecl_kw_config_struct {
  CONFIG_STD_FIELDS;
  char *ens_file;
};


enkf_ecl_kw_config_type * enkf_ecl_kw_config_alloc(enkf_var_type , int , const char * , const char *);
int        		  enkf_ecl_kw_config_get_size       (const enkf_ecl_kw_config_type *);
const char 		* enkf_ecl_kw_config_get_ensname_ref(const enkf_ecl_kw_config_type *);
void                      enkf_ecl_kw_config_free(enkf_ecl_kw_config_type *);

#endif
