#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <multz_config.h>
#include <mem_config.h>


struct multz_config_struct {
  int i1,i2,j1,j2;
  int nz;
  char * ecl_file;
  char * ens_file;
};


multz_config_type * multz_config_alloc(int nx, int ny , int nz , const char * ecl_file , const char * ens_file) {
  multz_config_type *multz_config = malloc(sizeof *multz_config);
  
  multz_config->i1 = 1;
  multz_config->j1 = 1;
  multz_config->i2 = nx;
  multz_config->j2 = ny;
  multz_config->nz = nz;

  multz_config->ecl_file = util_alloc_string_copy(ecl_file);
  multz_config->ens_file = util_alloc_string_copy(ens_file);
  
  return multz_config;
}


const char * multz_config_get_ensname_ref(const multz_config_type * multz_config) {
  return multz_config->ens_file;
}

const char * multz_config_get_eclname_ref(const multz_config_type * multz_config) {
  return multz_config->ecl_file;
}

int multz_config_get_nz(const multz_config_type *multz_config) { return multz_config->nz; }


void multz_config_fprintf_layer(const multz_config_type * multz_config , int layer , double multz_value , FILE *stream) {
  const int area = (multz_config->i2 - multz_config->i1 + 1) * (multz_config->j2 - multz_config->j1 + 1);
  fprintf(stream,"BOX\n   %5d %5d %5d %5d %5d %5d / \nMULTZ\n%d*%g /\nENDBOX\n\n" , multz_config->i1, multz_config->i2 , multz_config->j1 , multz_config->j2 , layer , layer , area , multz_value);
}



void multz_config_free(multz_config_type * multz_config) {
  free(multz_config->ecl_file);
  free(multz_config->ens_file);
  free(multz_config);
}
							 

