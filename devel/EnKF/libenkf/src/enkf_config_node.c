#include <enkf_types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <enkf_config_node.h> 

struct enkf_config_node_struct {
  config_free_ftype               *freef;
  config_get_serial_size_ftype    *get_serial_size;
  enkf_impl_type  	    	   impl_type;
  enkf_var_type  	    	   enkf_type; 

  void                     	  *data; /* This points to the config object of the actual implementation. */
} ;



enkf_config_node_type * enkf_config_node_alloc(enkf_var_type              enkf_type,
						      enkf_impl_type             impl_type,
						      const void               * data, 
						      config_free_ftype        * freef,
						      config_get_serial_size_ftype    * get_serial_size) {
  
  enkf_config_node_type * node = malloc( sizeof *node);
  node->data = (void *) data;
  node->freef       = freef;
  node->get_serial_size    = get_serial_size;
  node->enkf_type   = enkf_type;
  node->impl_type   = impl_type;

  return node;
}

void enkf_config_node_free(enkf_config_node_type * node) {
  if (node->freef != NULL) node->freef(node->data);
  free(node);
}

void enkf_config_node_free__(void * void_node) {
  enkf_config_node_type * node = (enkf_config_node_type *) void_node;
  enkf_config_node_free(node);
}

int enkf_config_node_get_serial_size(const enkf_config_node_type * config_node) {
  return config_node->get_serial_size(config_node->data);
}

bool enkf_config_node_include_type(const enkf_config_node_type * config_node , int mask) {
  if (config_node->enkf_type & mask)
    return true;
  else
    return false;
}

const void *  enkf_config_node_get_ref(const enkf_config_node_type * node) { 
  return node->data; 
}

enkf_impl_type enkf_config_node_get_impl_type(const enkf_config_node_type *config_node) { 
  return config_node->impl_type; 
}
