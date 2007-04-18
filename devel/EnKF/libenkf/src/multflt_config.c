#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <multflt_config.h>
#include <enkf_util.h>



multflt_config_type * multflt_config_alloc(int size, const char * ecl_file , const char * ens_file) {
  multflt_config_type *multflt_config = malloc(sizeof *multflt_config);
  
  multflt_config->size        = size;
  multflt_config->ecl_kw_name = NULL;
  multflt_config->var_type    = parameter;
  
  multflt_config->ecl_file    = util_alloc_string_copy(ecl_file);
  multflt_config->ens_file    = util_alloc_string_copy(ens_file);
  multflt_config->mean        = enkf_util_malloc(size * sizeof *multflt_config->mean        , __func__);
  multflt_config->std         = enkf_util_malloc(size * sizeof *multflt_config->std         ,  __func__);
  multflt_config->active      = enkf_util_malloc(size * sizeof *multflt_config->active      , __func__);
  multflt_config->fault_names = enkf_util_malloc(size * sizeof *multflt_config->fault_names , __func__);
  { 
    int i;
    for (i = 0; i < size; i++) {
      multflt_config->mean[i]   = 1.0;
      multflt_config->std[i]    = 0.25;
      multflt_config->active[i] = true;
      multflt_config->fault_names[i] = util_alloc_string_copy("FAULT");
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


void multflt_config_free(multflt_config_type * multflt_config) {
  free(multflt_config->ecl_file);
  free(multflt_config->ens_file);
  free(multflt_config->mean);
  free(multflt_config->std);
  free(multflt_config->active);
  util_free_string_list(multflt_config->fault_names , multflt_config->size);
  free(multflt_config);
}
							 

/*****************************************************************/
GET_SIZE_FUNC(multflt_config)
