#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <multflt_config.h>
#include <mem_config.h>


struct multflt_config_struct {
  int    nfaults;
  char * ecl_file;
  char * ens_file;
};


multflt_config_type * multflt_config_alloc(int nfaults, const char * ecl_file , const char * ens_file) {
  multflt_config_type *multflt_config = malloc(sizeof *multflt_config);
  
  multflt_config->nfaults  = nfaults;
  multflt_config->ecl_file = util_alloc_string_copy(ecl_file);
  multflt_config->ens_file = util_alloc_string_copy(ens_file);
  
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
  free(multflt_config);
}
							 

