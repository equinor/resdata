#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <enkf_config.h>


struct enkf_config_struct {
  int  		    ens_size;
  char 		   *ecl_root_path;
  char 		   *ens_root_path;
  char             *ens_path;
  char             *ecl_path;
  hash_type        *enkf_ecl_kw;  
};


/*****************************************************************/



bool enkf_config_restart_kw(const enkf_config_type *enkf_config, const char * ecl_kw_name) {
  if (hash_has_key(enkf_config->enkf_ecl_kw , ecl_kw_name))
    return true;
  else
    return false;
}


void enkf_config_add_enkf_kw(const enkf_config_type *enkf_config, const char *ecl_kw_name) {
  hash_insert_int(enkf_config->enkf_ecl_kw , ecl_kw_name , 1); 
}
								 

void enkf_config_set_ecl_root_path(enkf_config_type * enkf_config , const char * ecl_root_path) {
  enkf_config->ecl_root_path = util_realloc_string_copy(enkf_config->ecl_root_path , ecl_root_path);
  util_make_path(enkf_config->ecl_root_path);
}

void enkf_config_set_ens_root_path(enkf_config_type * enkf_config , const char * ens_root_path) {
  enkf_config->ens_root_path = util_realloc_string_copy(enkf_config->ens_root_path , ens_root_path);
  util_make_path(enkf_config->ens_root_path);
}


void enkf_config_set_ecl_path(enkf_config_type * enkf_config , const char * ecl_path) {
  enkf_config->ecl_path = util_realloc_full_path(enkf_config->ecl_path , enkf_config->ecl_root_path , ecl_path);
  util_make_path(enkf_config->ecl_path);
}


void enkf_config_set_ens_path(enkf_config_type * enkf_config , const char * ens_path) {
  enkf_config->ens_path = util_realloc_full_path(enkf_config->ens_path , enkf_config->ens_root_path , ens_path);
  util_make_path(enkf_config->ens_path);
}



enkf_config_type * enkf_config_alloc(const char * ens_root_path, const char *ecl_root_path) {
  enkf_config_type * enkf_config = malloc(sizeof *enkf_config);
  enkf_config->ens_root_path = NULL;
  enkf_config->ecl_root_path = NULL;
  enkf_config->enkf_ecl_kw = hash_alloc(10);
  enkf_config_set_ecl_root_path(enkf_config , ecl_root_path );
  enkf_config_set_ens_root_path(enkf_config , ens_root_path );
  enkf_config->ens_path = NULL;
  enkf_config->ecl_path = NULL;
  return enkf_config;
}


char * enkf_config_alloc_eclname(const enkf_config_type * enkf_config, const char * ext_name) {
  char * ecl_file = util_alloc_full_path(enkf_config->ecl_root_path , ext_name);
  return ecl_file;
}


void enkf_config_free(enkf_config_type * enkf_config) {
  /*
    if (enkf_config->ecl_root_path != NULL) free(enkf_config->ecl_root_path);
    if (enkf_config->ens_root_path != NULL) free(enkf_config->ens_root_path);
  */
  if (enkf_config->ecl_path != NULL) free(enkf_config->ecl_path);
  if (enkf_config->ens_path != NULL) free(enkf_config->ens_path);

  hash_free(enkf_config->enkf_ecl_kw);
  free(enkf_config);
}

