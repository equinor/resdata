#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <multz_config.h>
#include <mem_config.h>


struct multz_config_struct {
  int nx;
  int ny;
  int nz;
  char * ecl_file;
  char * ens_file;
};




multz_config_type * multz_config_alloc(int nx, int ny , int nz , const char * ecl_file , const char * ens_file) {
  multz_config_type *multz_config = malloc(sizeof *multz_config);
  
  multz_config->nx = nx;
  multz_config->ny = ny;
  multz_config->nz = nz;
  multz_config->ecl_file = util_alloc_string_copy(ecl_file);
  multz_config->ens_file = util_alloc_string_copy(ens_file);
  
  return multz_config;
}



const char * multz_config_get_ensname_ref(const multz_config_type * multz_config) {
  return multz_config->ens_file;
}



void multz_config_free(multz_config_type * multz_config) {
  free(multz_config->ecl_file);
  free(multz_config->ens_file);
  free(multz_config);
}
							 

