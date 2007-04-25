#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <util.h>
#include <ens_config.h>
#include <config.h>
#include <multz_config.h>





multz_config_type * multz_config_alloc(int nx , int ny , int nz , const char * ecl_name , const char * ens_name) {
  multz_config_type *config = malloc(sizeof *config);
  config->ecl_kw_name = NULL;
  config->size        = nz;
  config->var_type    = parameter;

  config->ecl_name = util_alloc_string_copy(ecl_name);
  config->ens_name = util_alloc_string_copy(ens_name);
  
  config->mean   = enkf_util_malloc(config->size * sizeof *config->mean   , __func__);
  config->std    = enkf_util_malloc(config->size * sizeof *config->std    , __func__);
  config->active = enkf_util_malloc(config->size * sizeof *config->active , __func__);
  config->i1     = enkf_util_malloc(config->size * sizeof *config->i1     , __func__);
  config->i2     = enkf_util_malloc(config->size * sizeof *config->i2     , __func__);
  config->j1     = enkf_util_malloc(config->size * sizeof *config->j1     , __func__);
  config->j2     = enkf_util_malloc(config->size * sizeof *config->j2     , __func__);
  config->k      = enkf_util_malloc(config->size * sizeof *config->k      , __func__);
  config->area   = enkf_util_malloc(config->size * sizeof *config->area   , __func__);
  { 
    int i;
    for (i = 0; i < config->size; i++) {
      config->mean[i]   = 1.0;
      config->std[i]    = 1.0;
      config->active[i] = true;

      config->i1[i]     = 1;
      config->i2[i]     = nx;

      config->j1[i]     = 1;
      config->j2[i]     = ny;

      config->k[i]      = i+1;
    }
  }

  { 
    int i;
    for (i = 0; i < config->size; i++) 
      config->area[i] = (config->i2[i]- config->i1[i] + 1) * (config->j2[i]- config->j1[i] + 1);
  }
  
  CONFIG_INIT_STD_FIELDS;
  return config;
}


const char * multz_config_get_ens_file_ref(const multz_config_type * config) {
  return config->ens_file;
}

const char * multz_config_get_ecl_file_ref(const multz_config_type * config) {
  return config->ecl_file;
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
CONFIG_SET_ECL_FILE(multz);
CONFIG_SET_ENS_FILE(multz);
CONFIG_SET_ECL_FILE_VOID(multz);
CONFIG_SET_ENS_FILE_VOID(multz);
CONFIG_GET_SIZE_FUNC(multz)

VOID_FUNC(multz_config_free , multz_config_type);

							 

