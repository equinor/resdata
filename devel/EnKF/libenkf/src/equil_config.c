#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <equil_config.h>
#include <enkf_util.h>



equil_config_type * equil_config_alloc(int size, const char * ecl_name , const char * ens_name) {
  equil_config_type *config = malloc(sizeof *config);
  
  config->size  = size;
  config->ecl_name = util_alloc_string_copy(ecl_name);
  config->ens_name = util_alloc_string_copy(ens_name);
  config->mean     = enkf_util_malloc(size * sizeof *config->mean , __func__);
  config->std      = enkf_util_malloc(size * sizeof *config->std ,  __func__);
  config->active   = enkf_util_malloc(size * sizeof *config->active , __func__);

  { 
    int i;
    for (i = 0; i < size; i++) {
      config->mean[i]   = 1.0;
      config->std[i]    = 0.25;
      config->active[i] = true;
    }
  }
  
  CONFIG_INIT_STD_FIELDS;
  return config;
}


const char * equil_config_get_ens_file_ref(const equil_config_type * equil_config) {
  return equil_config->ens_file;
}

const char * equil_config_get_ecl_file_ref(const equil_config_type * equil_config) {
  return equil_config->ecl_file;
}



void equil_config_free(equil_config_type * config) {
  CONFIG_FREE_STD_FIELDS;
  free(config->mean);
  free(config->std);
  free(config->active);
  free(config);
}
							 





/*****************************************************************/

CONFIG_SET_ECL_FILE(equil);
CONFIG_SET_ENS_FILE(equil);
CONFIG_SET_ECL_FILE_VOID(equil);
CONFIG_SET_ENS_FILE_VOID(equil);
CONFIG_GET_SIZE_FUNC(equil);

VOID_FUNC(equil_config_free , equil_config_type);
