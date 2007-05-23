#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <enkf_config.h>
#include <pathv.h>



/*
  path:

  ens_root_path/ens_path2 = full_ens_path

*/
  

struct enkf_config_struct {
  int  		    ens_size;
  hash_type        *config_hash;
  hash_type        *enkf_ecl_kw;  
  pathv_type       *enspath;
  int               eclpath_depth;
};


typedef struct {
  config_free_ftype        *freef;

  void *data;
} enkf_config_node_type;



/*****************************************************************/
/* enkf_config_node_type - entirely static. */

static enkf_config_node_type * enkf_config_node_alloc(const void *data, 
						      config_free_ftype        * freef ) {      
  
  enkf_config_node_type * node = malloc( sizeof *node);
  node->data = (void *) data;
  node->freef       = freef;
  
  return node;
}

static void enkf_config_node_free(enkf_config_node_type * node) {
  if (node->freef != NULL) node->freef(node->data);
  free(node);
}

static void enkf_config_node_free__(void * void_node) {
  enkf_config_node_type * node = (enkf_config_node_type *) void_node;
  enkf_config_node_free(node);
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



/*****************************************************************/

void enkf_config_iset_enspath(enkf_config_type * enkf_config , int i , const char *path) {
  pathv_iset(enkf_config->enspath , i , path);
}


int enkf_config_get_eclpath_depth(const enkf_config_type * enkf_config) { return enkf_config->eclpath_depth; }

enkf_config_type * enkf_config_alloc(int enspath_depth , int eclpath_depth) {
  enkf_config_type * enkf_config = malloc(sizeof *enkf_config);
  enkf_config->config_hash = hash_alloc(10);
  enkf_config->enkf_ecl_kw = hash_alloc(10);

  enkf_config->eclpath_depth = eclpath_depth;
  enkf_config->enspath       = pathv_alloc(enspath_depth , NULL);
  return enkf_config;
}



bool enkf_config_has_key(const enkf_config_type * enkf_config , const char * key) {
  return hash_has_key(enkf_config->config_hash , key);
}


void enkf_config_add_type(enkf_config_type * enkf_config, const char * key , const void *data , config_free_ftype * freef) {
  if (enkf_config_has_key(enkf_config , key)) {
    fprintf(stderr,"%s: a configuration object:%s has already been added - aborting \n",__func__ , key);
    abort();
  }
  {
    enkf_config_node_type * node = enkf_config_node_alloc(data , freef);
    hash_insert_hash_owned_ref(enkf_config->config_hash , key , node , enkf_config_node_free__);
  }
}


void enkf_config_free(enkf_config_type * enkf_config) {  
  pathv_free(enkf_config->enspath);
  hash_free(enkf_config->config_hash);
  hash_free(enkf_config->enkf_ecl_kw);
  free(enkf_config);
}


const void * enkf_config_get_ref(const enkf_config_type * config, const char * key) {
  if (hash_has_key(config->config_hash , key)) {
    enkf_config_node_type * node = hash_get(config->config_hash , key);
    return node->data;
  } else {
    fprintf(stderr,"%s: config node:%s does not exist \n",__func__ , key);
    abort();
  }
}



