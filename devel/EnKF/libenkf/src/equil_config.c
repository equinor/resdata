#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <equil_config.h>
#include <enkf_util.h>



equil_config_type * equil_config_alloc(int size, const char * eclfile , const char * ensfile) {
  equil_config_type *config = malloc(sizeof *config);
  
  config->size  = size;
  config->eclfile = util_alloc_string_copy(eclfile);
  config->ensfile = util_alloc_string_copy(ensfile);
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
  
  return config;
}


const char * equil_config_get_ensfile_ref(const equil_config_type * equil_config) {
  return equil_config->ensfile;
}

const char * equil_config_get_eclfile_ref(const equil_config_type * equil_config) {
  return equil_config->eclfile;
}



void equil_config_free(equil_config_type * config) {
  CONFIG_FREE_STD_FIELDS;
  free(config->mean);
  free(config->std);
  free(config->active);
  free(config);
}
							 





/*****************************************************************/

CONFIG_SET_ECLFILE(equil);
CONFIG_SET_ENSFILE(equil);
CONFIG_SET_ECLFILE_VOID(equil);
CONFIG_SET_ENSFILE_VOID(equil);
CONFIG_GET_SIZE_FUNC(equil);

VOID_FUNC(equil_config_free , equil_config_type);
