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


struct ecl_rft_vector_struct {
  hash_type * well_hash;
};



static ecl_rft_vector_type * ecl_rft_vector_alloc_empty() {
  ecl_rft_vector_type * rft_vector = malloc(sizeof * rft_vector);
  rft_vector->well_hash = hash_alloc(10);
  return rft_vector;
}



static void ecl_rft_vector_add_node(ecl_rft_vector_type * rft_vector , const ecl_rft_node_type * rft_node) {
  hash_insert_hash_owned_ref(rft_vector->well_hash , ecl_rft_node_well_name_ref(rft_node) , rft_node , ecl_rft_node_free__);
}




ecl_rft_vector_type * ecl_rft_vector_alloc(const char * filename , bool endian_convert) {
  ecl_rft_vector_type * rft_vector = ecl_rft_vector_alloc_empty();
  ecl_fstate_type     * fstate     = ecl_fstate_fread_alloc(1 , (const char **) &filename , ecl_rft_file , false , endian_convert);
  int size                         = ecl_fstate_get_size(fstate);
  int block_nr;

  for (block_nr = 0; block_nr < size; block_nr++) {
    ecl_rft_node_type *rft_node = ecl_rft_node_alloc(ecl_fstate_get_block(fstate , block_nr));
    if (rft_node != NULL) 
      ecl_rft_vector_add_node(rft_vector , rft_node);
  }
  
  ecl_fstate_free(fstate);
  return rft_vector;
}


bool ecl_rft_vector_has_well(const ecl_rft_vector_type * rft_vector , const char * well_name) {
  return hash_has_key(rft_vector->well_hash , well_name);
}


ecl_rft_node_type * ecl_rft_vector_get_node(const ecl_rft_vector_type * rft_vector , const char * well_name) {
  if (ecl_rft_vector_has_well(rft_vector , well_name))
    return hash_get(rft_vector->well_hash , well_name);
  else {
    fprintf(stderr,"%s: RFT vector does not have a well:%s - aborting \n",__func__ , well_name);
    abort();
  }
}


void ecl_rft_vector_block(const ecl_rft_vector_type * rft_vector , const char * well_name , int size , const double * tvd , int * i, int * j , int *k) {
  ecl_rft_node_block(ecl_rft_vector_get_node(rft_vector , well_name) , size , tvd, i , j , k);
}

void ecl_rft_vector_free(ecl_rft_vector_type * rft_vector) {
  hash_free(rft_vector->well_hash);
  free(rft_vector);
  rft_vector = NULL;
}


