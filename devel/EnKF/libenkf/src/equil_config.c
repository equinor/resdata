#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <ens_config.h>
#include <equil_config.h>
#include <enkf_util.h>



/*****************************************************************/

equil_config_type * equil_config_alloc(int size, bool WOC , bool GOC , const char * eclfile , const char * ensfile) {
  equil_config_type *config = malloc(sizeof *config);
  /* 
     Because WOC and GOC data are packed in the same data vector - can lead to bugs ... 
     Observe that the size variable in the rest of this function should refer to
     the number of equilibration regions, whereas config->size corresponds to this * 2
     
     => Can *NOT* tr/size/config->size/
  */

  config->data_size    = size * 2; 
  config->eclfile      = util_alloc_string_copy(eclfile);
  config->ensfile      = util_alloc_string_copy(ensfile);
  config->mean_WOC     = enkf_util_malloc(size * sizeof *config->mean_WOC , __func__);
  config->std_WOC      = enkf_util_malloc(size * sizeof *config->std_WOC ,  __func__);
  config->active_WOC   = enkf_util_malloc(size * sizeof *config->active_WOC , __func__);

  config->mean_GOC     = enkf_util_malloc(size * sizeof *config->mean_GOC , __func__);
  config->std_GOC      = enkf_util_malloc(size * sizeof *config->std_GOC ,  __func__);
  config->active_GOC   = enkf_util_malloc(size * sizeof *config->active_GOC , __func__);
  config->serial_size  = 0;
  
  { 
    int i;
    for (i = 0; i < size; i++) {
      config->mean_WOC[i]   = 1.0;
      config->std_WOC[i]    = 0.25;
      if (WOC) {
	config->active_WOC[i] = true;
	config->serial_size++;
      } else
	config->active_WOC[i] = false;

      config->mean_GOC[i]   = 1.0;
      config->std_GOC[i]    = 0.25;
      if (GOC) {
	config->active_GOC[i] = true;
	config->serial_size++;
      } else
	config->active_GOC[i] = false;
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


int equil_config_get_nequil(const equil_config_type * equil_config) {
  return equil_config->data_size / 2;
}
    

void equil_config_free(equil_config_type * config) {
  CONFIG_FREE_STD_FIELDS;
  free(config->mean_WOC);
  free(config->std_WOC);
  free(config->active_WOC);
  free(config->mean_GOC);
  free(config->std_GOC);
  free(config->active_GOC);
  free(config);
}
							 





/*****************************************************************/

CONFIG_SET_ECLFILE(equil);
CONFIG_SET_ENSFILE(equil);
CONFIG_SET_ECLFILE_VOID(equil);
CONFIG_SET_ENSFILE_VOID(equil);
GET_SERIAL_SIZE(equil);
VOID_GET_SERIAL_SIZE(equil);
GET_DATA_SIZE(equil);
VOID_FUNC(equil_config_free , equil_config_type);
