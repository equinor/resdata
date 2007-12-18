#include <enkf_types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <enkf_macros.h>
#include <enkf_config_node.h> 

struct enkf_config_node_struct {
  config_free_ftype               *freef;
  enkf_impl_type  	    	   impl_type;
  enkf_var_type  	    	   enkf_type; 
  char 		     * ensfile;          
  char 		     * eclfile;          
  config_set_serial_offset_ftype   * set_serial_offset;
  void                      	   * data; /* This points to the config object of the actual implementation. */
} ;



enkf_config_node_type * enkf_config_node_alloc(enkf_var_type              enkf_type,
					       enkf_impl_type             impl_type,
					       const void                      * data, 
					       config_free_ftype               * freef,
					       config_set_serial_offset_ftype  * set_serial_offset) {
  
  enkf_config_node_type * node = malloc( sizeof *node);
  node->data = (void *) data;
  node->freef              = freef;
  node->set_serial_offset  = set_serial_offset;
  node->enkf_type     	   = enkf_type;
  node->impl_type     	   = impl_type;
  node->ensfile            = NULL;
  node->eclfile            = NULL;

  return node;
}


void enkf_config_node_free(enkf_config_node_type * node) {
  if (node->freef   != NULL) node->freef(node->data);
  if (node->ensfile != NULL) free(node->ensfile);
  if (node->eclfile != NULL) free(node->eclfile);
  free(node);
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


VOID_FREE(enkf_config_node)
