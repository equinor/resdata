#include <stdlib.h>
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

  return multz_mem;
}


char * multz_mem_alloc_ensname(const multz_mem_type *multz_mem) {
  char *ens_name  = mem_config_alloc_ensname(multz_mem->mem_config , multz_config_get_ensname_ref(multz_mem->multz_config));
  return ens_name;
}


void multz_mem_free(multz_mem_type *multz_mem) {
  free(multz_mem);
}
