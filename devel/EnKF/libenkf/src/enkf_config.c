#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <multz_config.h>
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
  int               enspath_depth;
  int               eclpath_depth;
};


typedef struct {
  config_free_ftype        *freef;
  config_get_size_ftype    *get_size;
  void *data;
} enkf_config_node_type;



/*****************************************************************/
/* enkf_config_node_type - entirely static. */

static enkf_config_node_type * enkf_config_node_alloc(const void *data, 
						      config_free_ftype        * freef,
						      config_get_size_ftype    * get_size) {
  
  enkf_config_node_type * node = malloc( sizeof *node);
  node->data = (void *) data;
  node->freef       = freef;
  node->get_size    = get_size;
  
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

static int enkf_config_node_get_size(const enkf_config_node_type * config_node) {
  return config_node->get_size(config_node->data);
}


/*****************************************************************/

int enkf_config_get_data_size(const enkf_config_type * config) {
  int size = 0;
  int i;
  char **keylist = hash_alloc_keylist(config->config_hash);
  for (i= 0; i < hash_get_size(config->config_hash); i++) {
    enkf_config_node_type * config_node = hash_get(config->config_hash , keylist[i]);
    size += enkf_config_node_get_size(config_node);
  }
  hash_free_ext_keylist(config->config_hash , keylist);
  return size;
}


bool enkf_config_restart_kw(const enkf_config_type *enkf_config, const char * _ecl_kw_name) {
  char * ecl_kw_name = util_alloc_strip_copy(_ecl_kw_name);
  bool restart_kw;
  if (hash_has_key(enkf_config->enkf_ecl_kw , ecl_kw_name))
    restart_kw = true;
  else
    restart_kw = false;
  free(ecl_kw_name);
  return restart_kw;
}


void enkf_config_add_restart_type(const enkf_config_type *enkf_config, const char *_ecl_kw_name) {
  char * ecl_kw_name = util_alloc_strip_copy(_ecl_kw_name);
  hash_insert_int(enkf_config->enkf_ecl_kw , ecl_kw_name , 1); 
  free(ecl_kw_name);
}



/*****************************************************************/


int enkf_config_get_eclpath_depth(const enkf_config_type * enkf_config) { return enkf_config->eclpath_depth; }

int enkf_config_get_enspath_depth(const enkf_config_type * enkf_config) { return enkf_config->enspath_depth; }

enkf_config_type * enkf_config_alloc(int enspath_depth , int eclpath_depth) {
  enkf_config_type * enkf_config = malloc(sizeof *enkf_config);
  enkf_config->config_hash = hash_alloc(10);
  enkf_config->enkf_ecl_kw = hash_alloc(10);

  enkf_config->eclpath_depth = eclpath_depth;
  enkf_config->enspath_depth = enspath_depth;
  return enkf_config;
}



bool enkf_config_has_key(const enkf_config_type * enkf_config , const char * key) {
  return hash_has_key(enkf_config->config_hash , key);
}


void enkf_config_add_type(enkf_config_type * enkf_config, const char * key , const void *data , config_free_ftype * freef , config_get_size_ftype *get_size) {
  if (enkf_config_has_key(enkf_config , key)) {
    fprintf(stderr,"%s: a configuration object:%s has already been added - aborting \n",__func__ , key);
    abort();
  }
  {
    enkf_config_node_type * node = enkf_config_node_alloc(data , freef , get_size);
    hash_insert_hash_owned_ref(enkf_config->config_hash , key , node , enkf_config_node_free__);
  }
}


void enkf_config_free(enkf_config_type * enkf_config) {  
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



