#include <stdlib.h>
#include <stdio.h>
#include <enkf_types.h>
#include <multz_config.h>
#include <multz.h>


struct multz_struct {
  const multz_config_type *multz_config;
  const mem_config_type   *mem_config;
  enkf_float_type         *data;
};

/*****************************************************************/

multz_type * multz_alloc(const mem_config_type * mem_config , const multz_config_type * multz_config) {
  multz_type * multz = malloc(sizeof *multz);
  multz->mem_config   = mem_config;
  multz->multz_config = multz_config;
  multz->data         = calloc(multz_config_get_nz(multz_config) , sizeof *multz->data);
  return multz;
}



char * multz_alloc_ensname(const multz_type *multz) {
  char *ens_name  = mem_config_alloc_ensname(multz->mem_config , multz_config_get_ensname_ref(multz->multz_config));
  return ens_name;
}



char * multz_alloc_eclname(const multz_type *multz) {
  char  *ecl_name = mem_config_alloc_eclname(multz->mem_config , multz_config_get_eclname_ref(multz->multz_config));
  return ecl_name;
}


void multz_ecl_write(const multz_type * multz) {
  char * ecl_file = multz_alloc_eclname(multz);
  FILE * stream   = fopen(ecl_file , "w");
  if (stream == NULL) {
    fprintf(stderr,"%s: failed to open:%s for writing - aborting \n",__func__ , ecl_file);
    free(ecl_file);
    abort();
  }
  printf("Har aapnet: %s \n",ecl_file);
  {
    int k;
    for (k=0; k < multz_config_get_nz(multz->multz_config); k++)
      multz_config_fprintf_layer(multz->multz_config , k + 1 , multz->data[k] , stream);
  }
  
  fclose(stream);
  free(ecl_file);
}




void multz_free(multz_type *multz) {
  free(multz);
}
