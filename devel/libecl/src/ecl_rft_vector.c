#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <ecl_rft_vector.h>
#include <ecl_rft_node.h>
#include <hash.h>
#include <vector.h>

struct ecl_rft_vector_struct {
  char        * filename;
  vector_type * data;
};



static ecl_rft_vector_type * ecl_rft_vector_alloc_empty(const char * filename) {
  ecl_rft_vector_type * rft_vector = util_malloc(sizeof * rft_vector , __func__);
  rft_vector->data     = vector_alloc_new();
  rft_vector->filename  = util_alloc_string_copy(filename);
  return rft_vector;
}



static void ecl_rft_vector_add_node(ecl_rft_vector_type * rft_vector , const ecl_rft_node_type * rft_node) {
  vector_append_owned_ref( rft_vector->data , rft_node , ecl_rft_node_free__);
}




ecl_rft_vector_type * ecl_rft_vector_alloc(const char * filename) {
  bool endian_flip;
  fortio_guess_endian_flip( filename , &endian_flip);
  {
    ecl_rft_vector_type * rft_vector = ecl_rft_vector_alloc_empty(filename);
    ecl_fstate_type     * fstate     = ecl_fstate_fread_alloc(1 , (const char **) &filename , ecl_rft_file , endian_flip , false);
    int size                         = ecl_fstate_get_size(fstate);
    int block_nr;
    
    for (block_nr = 0; block_nr < size; block_nr++) {
      ecl_rft_node_type *rft_node = ecl_rft_node_alloc(ecl_fstate_iget_block(fstate , block_nr));
      if (rft_node != NULL) 
	ecl_rft_vector_add_node(rft_vector , rft_node);
    }
    
    ecl_fstate_free(fstate);
    return rft_vector;
  }
}




void ecl_rft_vector_free(ecl_rft_vector_type * rft_vector) {
  vector_free(rft_vector->data);
  free(rft_vector->filename);
  free(rft_vector);
}


void ecl_rft_vector_summarize(const ecl_rft_vector_type * rft_vector , bool show_completions) {
  int iw;
  for (iw = 0; iw < vector_get_size( rft_vector->data ); iw++) {
    ecl_rft_node_summarize(vector_iget( rft_vector->data , iw) , show_completions);
    printf("\n");
  }
}

