#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <ecl_config.h>

struct ecl_config_struct {
  char 		   *root_path;
};


/*****************************************************************/




								 

void ecl_config_set_root_path(ecl_config_type * ecl_config , const char * root_path) {

  if (root_path == NULL) {
    fprintf(stderr,"%s: root_path == NULL - invalid - aborting \n",__func__);
    abort();
  }
  ecl_config->root_path = util_realloc_string_copy(ecl_config->root_path , root_path);
  util_make_path(ecl_config->root_path);
}



ecl_config_type * ecl_config_alloc(const char * root_path) {
  ecl_config_type * ecl_config = malloc(sizeof *ecl_config);
  ecl_config->root_path = NULL;
  ecl_config_set_root_path(ecl_config , root_path );
  return ecl_config;
}


char * ecl_config_alloc_eclname(const ecl_config_type * ecl_config, const char * ext_name) {
  char * ecl_file = util_alloc_full_path(ecl_config->root_path , ext_name);
  return ecl_file;
}


void ecl_config_free(ecl_config_type * ecl_config) {
  if (ecl_config->root_path != NULL) free(ecl_config->root_path);
  free(ecl_config);
}

