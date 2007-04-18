#include <stdlib.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <util.h>
#include <ens_config.h>
#include <multz_config.h>





multz_config_type * multz_config_alloc(int nx, int ny , int nz , const char * ecl_file , const char * ens_file) {
  multz_config_type *multz_config = malloc(sizeof *multz_config);
  multz_config->ecl_kw_name = NULL;
  multz_config->size        = nz;
  multz_config->var_type    = parameter;

  multz_config->ecl_file = util_alloc_string_copy(ecl_file);
  multz_config->ens_file = util_alloc_string_copy(ens_file);
  
  multz_config->mean   = enkf_util_malloc(multz_config->size * sizeof *multz_config->mean   , __func__);
  multz_config->std    = enkf_util_malloc(multz_config->size * sizeof *multz_config->std    , __func__);
  multz_config->active = enkf_util_malloc(multz_config->size * sizeof *multz_config->active , __func__);
  multz_config->i1     = enkf_util_malloc(multz_config->size * sizeof *multz_config->i1     , __func__);
  multz_config->i2     = enkf_util_malloc(multz_config->size * sizeof *multz_config->i2     , __func__);
  multz_config->j1     = enkf_util_malloc(multz_config->size * sizeof *multz_config->j1     , __func__);
  multz_config->j2     = enkf_util_malloc(multz_config->size * sizeof *multz_config->j2     , __func__);
  multz_config->k      = enkf_util_malloc(multz_config->size * sizeof *multz_config->k      , __func__);
  multz_config->area   = enkf_util_malloc(multz_config->size * sizeof *multz_config->area   , __func__);
  { 
    int i;
    for (i = 0; i < multz_config->size; i++) {
      multz_config->mean[i]   = 1.0;
      multz_config->std[i]    = 1.0;
      multz_config->active[i] = true;

      multz_config->i1[i]     = 1;
      multz_config->i2[i]     = nx;

      multz_config->j1[i]     = 1;
      multz_config->j2[i]     = ny;

      multz_config->k[i]      = i+1;
    }
  }

  { 
    int i;
    for (i = 0; i < multz_config->size; i++) 
      multz_config->area[i] = (multz_config->i2[i]- multz_config->i1[i] + 1) * (multz_config->j2[i]- multz_config->j1[i] + 1);
  }

  return multz_config;
}


const char * multz_config_get_ensname_ref(const multz_config_type * multz_config) {
  return multz_config->ens_file;
}

const char * multz_config_get_eclname_ref(const multz_config_type * multz_config) {
  return multz_config->ecl_file;
}

void multz_config_fprintf_layer(const multz_config_type * multz_config , int ik , double multz_value , FILE *stream) {
  fprintf(stream,"BOX\n   %5d %5d %5d %5d %5d %5d / \nMULTZ\n%d*%g /\nENDBOX\n\n" , 
	  multz_config->i1[ik]   , multz_config->i2[ik] , 
	  multz_config->j1[ik]   , multz_config->j2[ik] , 
	  multz_config->k[ik]    , multz_config->k[ik]  , 
	  multz_config->area[ik] , multz_value);
}



void multz_config_free(multz_config_type * multz_config) {
  free(multz_config->ecl_file);
  free(multz_config->ens_file);
  free(multz_config->mean);
  free(multz_config->std);
  free(multz_config->active);
  free(multz_config->j1);
  free(multz_config->j2);
  free(multz_config->i1);
  free(multz_config->i2);
  free(multz_config->k);
  free(multz_config->area);
  free(multz_config);
}


/*****************************************************************/
GET_SIZE_FUNC(multz_config)

							 

