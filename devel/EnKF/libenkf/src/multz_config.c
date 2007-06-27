#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <util.h>
#include <ens_config.h>
#include <config.h>
#include <multz_config.h>





multz_config_type * multz_config_alloc(int nx , int ny , int nz , const char * eclfile , const char * ensfile) {
  multz_config_type *config = malloc(sizeof *config);
  config->ecl_kw_name = NULL;
  config->data_size   = nz;
  config->var_type    = parameter;

  config->eclfile = util_alloc_string_copy(eclfile);
  config->ensfile = util_alloc_string_copy(ensfile);

  config->mean   = enkf_util_malloc(config->data_size * sizeof *config->mean   , __func__);
  config->std    = enkf_util_malloc(config->data_size * sizeof *config->std    , __func__);
  config->active = enkf_util_malloc(config->data_size * sizeof *config->active , __func__);
  config->i1     = enkf_util_malloc(config->data_size * sizeof *config->i1     , __func__);
  config->i2     = enkf_util_malloc(config->data_size * sizeof *config->i2     , __func__);
  config->j1     = enkf_util_malloc(config->data_size * sizeof *config->j1     , __func__);
  config->j2     = enkf_util_malloc(config->data_size * sizeof *config->j2     , __func__);
  config->k      = enkf_util_malloc(config->data_size * sizeof *config->k      , __func__);
  config->area   = enkf_util_malloc(config->data_size * sizeof *config->area   , __func__);
  config->serial_size = 0;
  { 
    int i;
    for (i = 0; i < config->data_size; i++) {
      config->mean[i]   = 1.0;
      config->std[i]    = 1.0;
      config->active[i] = true;

      config->i1[i]     = 1;
      config->i2[i]     = nx;

      config->j1[i]     = 1;
      config->j2[i]     = ny;

      config->k[i]      = i+1;

      if (config->active[i])
	config->serial_size++;
    }
  }

  { 
    int i;
    for (i = 0; i < config->data_size; i++) 
      config->area[i] = (config->i2[i]- config->i1[i] + 1) * (config->j2[i]- config->j1[i] + 1);
  }
  
  return config;
}


const char * multz_config_get_ensfile_ref(const multz_config_type * config) {
  return config->ensfile;
}

const char * multz_config_get_eclfile_ref(const multz_config_type * config) {
  return config->eclfile;
}


void multz_config_fprintf_layer(const multz_config_type * config , int ik , double multz_value , FILE *stream) {
  fprintf(stream,"BOX\n   %5d %5d %5d %5d %5d %5d / \nMULTZ\n%d*%g /\nENDBOX\n\n" , 
	  config->i1[ik]   , config->i2[ik] , 
	  config->j1[ik]   , config->j2[ik] , 
	  config->k[ik]    , config->k[ik]  , 
	  config->area[ik] , multz_value);
}



void multz_config_free(multz_config_type * config) {
  free(config->std);
  free(config->mean);
  free(config->active);
  free(config->j1);
  free(config->j2);
  free(config->i1);
  free(config->i2);
  free(config->k);
  free(config->area);
  CONFIG_FREE_STD_FIELDS;
  free(config);
}


/*****************************************************************/
CONFIG_SET_ECLFILE(multz);
CONFIG_SET_ENSFILE(multz);
CONFIG_SET_ECLFILE_VOID(multz);
CONFIG_SET_ENSFILE_VOID(multz);
GET_SERIAL_SIZE(multz)
GET_DATA_SIZE(multz)
VOID_GET_SERIAL_SIZE(multz)

VOID_FUNC(multz_config_free , multz_config_type);

							 

