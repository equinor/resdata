#include <stdlib.h>
#include <util.h>
#include <enkf_util.h>
#include <enkf_ecl_kw_config.h>




enkf_ecl_kw_config_type * enkf_ecl_kw_config_alloc(int size , const char *ecl_kw_name , const char * ens_file) {
  enkf_ecl_kw_config_type *config = malloc(sizeof *config);
  config->size        = size;
  config->ecl_kw_name = util_alloc_string_copy(ecl_kw_name);
  config->ens_file    = util_alloc_string_copy(ens_file);
  return config;
}




void enkf_ecl_kw_config_free(enkf_ecl_kw_config_type *config) {
  free(config->ecl_kw_name);
  free(config->ens_file);
  free(config);
}


