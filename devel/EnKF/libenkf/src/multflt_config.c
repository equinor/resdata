#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <multflt_config.h>
#include <enkf_util.h>



multflt_config_type * multflt_config_alloc(int nfaults, const char * ecl_file , const char * ens_file) {
  multflt_config_type *multflt_config = malloc(sizeof *multflt_config);
  
  multflt_config->nfaults  = nfaults;
  multflt_config->ecl_file = util_alloc_string_copy(ecl_file);
  multflt_config->ens_file = util_alloc_string_copy(ens_file);
  multflt_config->mean     = enkf_util_malloc(nfaults * sizeof *multflt_config->mean , __func__);
  multflt_config->std      = enkf_util_malloc(nfaults * sizeof *multflt_config->std ,  __func__);
  multflt_config->active   = enkf_util_malloc(nfaults * sizeof *multflt_config->active , __func__);

  { 
    int i;
    for (i = 0; i < nfaults; i++) {
      multflt_config->mean[i]   = 1.0;
      multflt_config->std[i]    = 0.25;
      multflt_config->active[i] = true;
    }
  }
  
  return multflt_config;
}


const char * multflt_config_get_ensname_ref(const multflt_config_type * multflt_config) {
  return multflt_config->ens_file;
}

const char * multflt_config_get_eclname_ref(const multflt_config_type * multflt_config) {
  return multflt_config->ecl_file;
}

int multflt_config_get_nfaults(const multflt_config_type *multflt_config) { return multflt_config->nfaults; }


void multflt_config_free(multflt_config_type * multflt_config) {
  free(multflt_config->ecl_file);
  free(multflt_config->ens_file);
  free(multflt_config->mean);
  free(multflt_config->std);
  free(multflt_config->active);
  free(multflt_config);
}
							 

