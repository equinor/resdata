#include <stdlib.h>
#include <drivers.h>
#include <enkf_node.h>

/******************************************************************/
/* Plain vanilla */

void drivers_plain_load_node(enkf_node_type * node, const char * filename) {
  enkf_node_fread(node , filename);
}


void drivers_plain_save_node(enkf_node_type * node, const char * filename) {
  enkf_node_fwrite(node , filename);
}


void drivers_plain_swapout_node(enkf_node_type * node, const char * filename) {
  enkf_node_swapout(node , filename);
}


void drivers_plain_swapin_node(enkf_node_type * node, const char * filename) {
  enkf_node_swapin(node , filename);
}



/******************************************************************/
