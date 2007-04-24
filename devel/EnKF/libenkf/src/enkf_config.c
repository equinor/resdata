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
  hash_type        *config_hash;
  hash_type        *enkf_ecl_kw;  
};


typedef struct {

  config_set_ensfile_ftype *set_ensfile;
  config_set_eclfile_ftype *set_eclfile;
  config_free_ftype        *freef;

  const void *data;
} enkf_config_node_type;



/*****************************************************************/
/* enkf_config_node_type - entirely static. */

static enkf_config_node_type * enkf_config_node_alloc(const void *data, 
					       config_set_ensfile_ftype * set_ensfile , 
					       config_set_eclfile_ftype * set_eclfile , 
					       config_free_ftype        * freef ) {      

  enkf_config_node_type * node = malloc( sizeof *node);
  node->data = data;
  node->set_ensfile = set_ensfile;
  node->set_eclfile = set_eclfile;
  node->freef       = freef;
  
  return node;
}

static void enkf_config_node_free(enkf_config_node_type * node) {
  if (node->freef != NULL) node->freef(node->data);
  free(node);
}

static void enkf_config_node_set_eclfile(enkf_config_node_type * node) {
  if (node->set_eclfile == NULL) {
    fprintf(stderr,"%s: called with set_eclfile function pointer=NULL - aborting \n",__func__);
    abort();
  }
  node->set_eclfile(node->data);
}

static void enkf_config_node_set_ensfile(enkf_config_node_type * node) {
  if (node->set_ensfile == NULL) {
    fprintf(stderr,"%s: called with set_ensfile function pointer=NULL - aborting \n",__func__);
    abort();
  }
  node->set_ensfile(node->data);
}


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
  enkf-config->config_hash = hash_alloc(10);
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



void enkf_config_add_type(enkf_config_type * enkf_config, const char * key , const void *data ,  
			  config_set_ensfile_ftype * set_ensfile , 
			  config_set_eclfile_ftype * set_eclfile , 
			  config_free_ftype        * freef ) {
  if (hash_has_key(enkf_config->config_hash , key)) {
    fprintf(stderr,"%s: a configuration object:%s has already been added - aborting \n",__func__);
    abort();
  }
  {
    enkf_config_node_type * node = enkf_config_node_alloc(data , set_ensfile , set_eclfile, freef);
    hash_insert_managed_ref(enkf_config->config_hash , node , key);
  }
}


void enkf_config_free(enkf_config_type * enkf_config) {
  /*
    if (enkf_config->ecl_root_path != NULL) free(enkf_config->ecl_root_path);
    if (enkf_config->ens_root_path != NULL) free(enkf_config->ens_root_path);
  */
  if (enkf_config->ecl_path != NULL) free(enkf_config->ecl_path);
  if (enkf_config->ens_path != NULL) free(enkf_config->ens_path);

  hash_free(enkf_config->config_hash);
  hash_free(enkf_config->enkf_ecl_kw);
  free(enkf_config);
}


