#include <stdlib.h>
#include <util.h>
#include <ens_config.h>

struct ens_config_struct {
  int size;
  char * ens_path;
};



ens_config_type * ens_config_alloc(int size , const char * ens_path) {
  ens_config_type * ens_config = malloc(sizeof *ens_config);
  ens_config->size     = size;
  ens_config->ens_path = util_alloc_string_copy(ens_path);
  util_make_path(ens_path);
  return ens_config;
}


char * ens_config_alloc_ensname(const ens_config_type * ens_config, const char * ext_name) {
  char * ens_file = util_alloc_full_path(ens_config->ens_path , ext_name);
  return ens_file;
}


void ens_config_free(ens_config_type * ens_config) {
  free(ens_config->ens_path);
  free(ens_config);
}

