#include <stdlib.h>
#include <util.h>
#include <ens_config.h>
#include <mem_config.h>


struct mem_config_struct {
  const ens_config_type * ens_config;
  char * ens_path;
  char * ecl_path;
};



void mem_config_set_ens_path(mem_config_type * mem_config, const char * ens_path) {
  mem_config->ens_path = util_realloc_string_copy(mem_config->ens_path , ens_path);
}


void mem_config_set_ecl_path(mem_config_type * mem_config, const char * ecl_path) {
  mem_config->ecl_path = util_realloc_string_copy(mem_config->ecl_path , ecl_path);
}


mem_config_type * mem_config_alloc(const ens_config_type * ens_config , const char *ecl_path , const char *ens_path) {
  mem_config_type * mem_config = malloc(sizeof *mem_config);
  mem_config->ens_path = NULL;
  mem_config->ecl_path = NULL;

  mem_config_set_ens_path(mem_config , ens_path);
  mem_config_set_ecl_path(mem_config , ecl_path);
  mem_config->ens_config = ens_config;

  return mem_config;
}


char * mem_config_alloc_ensname(const mem_config_type * mem_config, const char * ext_name) {
  char *tmp_name = util_alloc_full_path(mem_config->ens_path , ext_name);
  char *ens_file = ens_config_alloc_ensname(mem_config->ens_config , tmp_name);
  free(tmp_name);
  return ens_file;
}


void mem_config_free(mem_config_type *mem_config) {
  free(mem_config->ens_path);
  free(mem_config->ecl_path);
  free(mem_config);
}

