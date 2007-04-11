#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <equil_config.h>
#include <enkf_util.h>



equil_config_type * equil_config_alloc(int nequil, const char * ecl_file , const char * ens_file) {
  equil_config_type *equil_config = malloc(sizeof *equil_config);
  
  equil_config->nequil  = nequil;
  equil_config->ecl_file = util_alloc_string_copy(ecl_file);
  equil_config->ens_file = util_alloc_string_copy(ens_file);
  equil_config->mean     = enkf_util_malloc(nequil * sizeof *equil_config->mean , __func__);
  equil_config->std      = enkf_util_malloc(nequil * sizeof *equil_config->std ,  __func__);
  equil_config->active   = enkf_util_malloc(nequil * sizeof *equil_config->active , __func__);

  { 
    int i;
    for (i = 0; i < nequil; i++) {
      equil_config->mean[i]   = 1.0;
      equil_config->std[i]    = 0.25;
      equil_config->active[i] = true;
    }
  }
  
  return equil_config;
}


const char * equil_config_get_ensname_ref(const equil_config_type * equil_config) {
  return equil_config->ens_file;
}

const char * equil_config_get_eclname_ref(const equil_config_type * equil_config) {
  return equil_config->ecl_file;
}

int equil_config_get_nequil(const equil_config_type *equil_config) { return equil_config->nequil; }


void equil_config_free(equil_config_type * equil_config) {
  free(equil_config->ecl_file);
  free(equil_config->ens_file);
  free(equil_config->mean);
  free(equil_config->std);
  free(equil_config->active);
  free(equil_config);
}
							 

