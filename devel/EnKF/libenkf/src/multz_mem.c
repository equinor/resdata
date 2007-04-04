#include <stdlib.h>
#include <stdio.h>
#include <enkf_types.h>
#include <multz_config.h>
#include <multz_mem.h>


struct multz_mem_struct {
  const multz_config_type *multz_config;
  const mem_config_type   *mem_config;
  enkf_float_type         *data;
};

/*****************************************************************/

multz_mem_type * multz_mem_alloc(const mem_config_type * mem_config , const multz_config_type * multz_config) {
  multz_mem_type * multz_mem = malloc(sizeof *multz_mem);
  multz_mem->mem_config   = mem_config;
  multz_mem->multz_config = multz_config;
  multz_mem->data         = calloc(multz_config_get_nz(multz_config) , sizeof *multz_mem->data);
  return multz_mem;
}



char * multz_mem_alloc_ensname(const multz_mem_type *multz_mem) {
  char *ens_name  = mem_config_alloc_ensname(multz_mem->mem_config , multz_config_get_ensname_ref(multz_mem->multz_config));
  return ens_name;
}



char * multz_mem_alloc_eclname(const multz_mem_type *multz_mem) {
  char  *ecl_name = mem_config_alloc_eclname(multz_mem->mem_config , multz_config_get_eclname_ref(multz_mem->multz_config));
  return ecl_name;
}


void multz_mem_ecl_write(const multz_mem_type * multz_mem) {
  char * ecl_file = multz_mem_alloc_eclname(multz_mem);
  FILE * stream   = fopen(ecl_file , "w");
  if (stream == NULL) {
    fprintf(stderr,"%s: failed to open:%s for writing - aborting \n",__func__ , ecl_file);
    free(ecl_file);
    abort();
  }
  printf("Har aapnet: %s \n",ecl_file);
  {
    int k;
    for (k=0; k < multz_config_get_nz(multz_mem->multz_config); k++)
      multz_config_fprintf_layer(multz_mem->multz_config , k + 1 , multz_mem->data[k] , stream);
  }
  
  fclose(stream);
  free(ecl_file);
}




void multz_mem_free(multz_mem_type *multz_mem) {
  free(multz_mem);
}
