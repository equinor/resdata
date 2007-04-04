#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <mem_config.h>
#include <ens_config.h>
#include <ecl_config.h>



struct mem_config_struct {
  const ens_config_type * ens_config;
  const ecl_config_type * ecl_config;
  char * mem_ens_path; 
  char * mem_ecl_path;
};



void mem_config_set_ens_path(mem_config_type * mem_config, const char * ens_path) {
  mem_config->mem_ens_path = util_realloc_string_copy(mem_config->mem_ens_path , ens_path);
}


void mem_config_set_ecl_path(mem_config_type * mem_config, const char * ecl_path) {
  mem_config->mem_ecl_path = util_realloc_string_copy(mem_config->mem_ecl_path , ecl_path);
}




mem_config_type * mem_config_alloc(const ens_config_type * ens_config , const ecl_config_type * ecl_config , const char *ens_path, const char *ecl_path) {
  mem_config_type * mem_config = malloc(sizeof *mem_config);
  mem_config->mem_ens_path = NULL;
  mem_config->mem_ecl_path = NULL;

  mem_config_set_ens_path(mem_config , ens_path);
  mem_config_set_ecl_path(mem_config , ecl_path);

  mem_config->ens_config = ens_config;
  mem_config->ecl_config = ecl_config;

  return mem_config;
}


char * mem_config_alloc_ensname(const mem_config_type * mem_config, const char * ext_name) {
  char *path     = ens_config_alloc_ensname(mem_config->ens_config , mem_config->mem_ens_path);
  char *ens_file = util_alloc_full_path(path , ext_name);

  util_make_path(path);
  free(path);

  return ens_file;
}


char * mem_config_alloc_eclname(const mem_config_type * mem_config, const char * ext_name) {
  char *path     = ecl_config_alloc_eclname(mem_config->ecl_config , mem_config->mem_ecl_path);
  char *ecl_file = util_alloc_full_path(path , ext_name);
  free(path);
  return ecl_file;
}


void mem_config_make_ecl_path(const mem_config_type *mem_config) {
  char *path = ecl_config_alloc_eclname(mem_config->ecl_config , mem_config->mem_ecl_path);
  util_make_path(path);
  free(path);
}


void mem_config_unlink_ecl_path(const mem_config_type *mem_config) {
  char *path = ecl_config_alloc_eclname(mem_config->ecl_config , mem_config->mem_ecl_path);
  util_unlink_path(path);
  free(path);
}




void mem_config_free(mem_config_type *mem_config) {
  free(mem_config->mem_ens_path);
  free(mem_config->mem_ecl_path);
  free(mem_config);
}

