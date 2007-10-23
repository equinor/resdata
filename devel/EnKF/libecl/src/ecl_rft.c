#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_fstate.h>
#include <ecl_rft.h>
#include <hash.h>


struct ecl_rft_vector_struct {
  hash_type * well_hash;
};



static ecl_rft_vector_type * ecl_rft_vector_alloc_empty() {
  ecl_rft_vector_type * rft_vector = malloc(sizeof * rft_vector);
  rft_vector->well_hash = hash_alloc(10);
  return rft_vector;
}



ecl_rft_vector_type * ecl_rft_vector_alloc(const char * filename , bool endian_convert) {
  ecl_rft_vector_type * rft_vector = ecl_rft_vector_alloc_empty();
  ecl_fstate_type     * fstate     = ecl_fstate_fread_alloc(1 , (const char **) &filename , ecl_rft_file , false , endian_convert);

  
  
  ecl_fstate_free(fstate);
  return rft_vector;
}

