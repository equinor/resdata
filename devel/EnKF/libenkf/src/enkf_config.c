#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <multz_config.h>
#include <enkf_config.h>
#include <enkf_config_node.h>
#include <pathv.h>
#include <ecl_static_kw_config.h>
#include <enkf_types.h>


  

struct enkf_config_struct {
  int  		    ens_size;
  hash_type        *config_hash;
  int               enspath_depth;
  int               eclpath_depth;
  bool              endian_swap;
};




/*****************************************************************/





static int enkf_config_get_serial_size__(const enkf_config_type * config , int mask) {
  int size = 0;
  int i;
  char **keylist = hash_alloc_keylist(config->config_hash);
  for (i= 0; i < hash_get_size(config->config_hash); i++) {
    enkf_config_node_type * config_node = hash_get(config->config_hash , keylist[i]);
    if (enkf_config_node_include_type(config_node , mask))
      size += enkf_config_node_get_serial_size(config_node);
  }
  hash_free_ext_keylist(config->config_hash , keylist);
  return size;
}


int enkf_config_get_serial_size(const enkf_config_type * config) {
  return enkf_config_get_serial_size__(config , parameter + ecl_restart + ecl_summary);
}


enkf_impl_type enkf_config_impl_type(const enkf_config_type *enkf_config, const char * ecl_kw_name) {
  enkf_impl_type impl_type;

  if (hash_has_key(enkf_config->config_hash , ecl_kw_name)) {
    enkf_config_node_type * node = hash_get(enkf_config->config_hash , ecl_kw_name);
    impl_type = enkf_config_node_get_impl_type(node);
  } else
    impl_type = STATIC;

  return impl_type;
}



/*****************************************************************/

bool enkf_config_get_endian_swap(const enkf_config_type * enkf_config) { return enkf_config->endian_swap; }

int enkf_config_get_eclpath_depth(const enkf_config_type * enkf_config) { return enkf_config->eclpath_depth; }

int enkf_config_get_enspath_depth(const enkf_config_type * enkf_config) { return enkf_config->enspath_depth; }

enkf_config_type * enkf_config_alloc(int enspath_depth , int eclpath_depth, bool endian_swap) {
  enkf_config_type * enkf_config = malloc(sizeof *enkf_config);
  enkf_config->config_hash = hash_alloc(10);
  
  enkf_config->eclpath_depth = eclpath_depth;
  enkf_config->enspath_depth = enspath_depth;
  enkf_config->endian_swap   = endian_swap;
  return enkf_config;
}



bool enkf_config_has_key(const enkf_config_type * enkf_config , const char * key) {
  return hash_has_key(enkf_config->config_hash , key);
}





void enkf_config_add_type(enkf_config_type * enkf_config, 
			  const char * key , 
			  enkf_var_type enkf_type , 
			  enkf_impl_type impl_type , 
			  const void *data , 
			  config_free_ftype * freef , 
			  config_get_serial_size_ftype *get_serial_size) {
  if (enkf_config_has_key(enkf_config , key)) {
    fprintf(stderr,"%s: a configuration object:%s has already been added - aborting \n",__func__ , key);
    abort();
  }
  {
    enkf_config_node_type * node = enkf_config_node_alloc(enkf_type , impl_type , data , freef , get_serial_size);
    hash_insert_hash_owned_ref(enkf_config->config_hash , key , node , enkf_config_node_free__);
    printf("Setter:%s -> %d \n",key , impl_type);
  }
}



void enkf_config_add_type0(enkf_config_type * enkf_config , const char *key , int size, enkf_var_type enkf_type , enkf_impl_type impl_type) {
  switch(impl_type) {
  case(STATIC):
    enkf_config_add_type(enkf_config , key , enkf_type , impl_type , ecl_static_kw_config_alloc(size , key , key) , ecl_static_kw_config_free__ , ecl_static_kw_config_get_serial_size__);
    break;
  case(FIELD):
    /*
      enkf_config_add_type(enkf_config , key , enkf_type , impl_type , field_config_alloc(size , key , key)   , field_config_free__ , field_config_get_size__);
    */
    fprintf(stderr,"%s: Can not add FIELD config objects like:%s on the run - these must be from the main program with enkf_config_add_type - sorry.\n",__func__ , key);
    abort();
    break;
  default:
    fprintf(stderr,"%s only STATIC and FIELD types are implemented - aborting \n",__func__);
    abort();
  }
}



void enkf_config_free(enkf_config_type * enkf_config) {  
  hash_free(enkf_config->config_hash);
  free(enkf_config);
}


const void * enkf_config_get_ref(const enkf_config_type * config, const char * key) {
  if (hash_has_key(config->config_hash , key)) {
    enkf_config_node_type * node = hash_get(config->config_hash , key);
    return node;
  } else {
    fprintf(stderr,"%s: config node:%s does not exist \n",__func__ , key);
    abort();
  }
}



