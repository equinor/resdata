#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <util.h>
#include <ecl_rft_file.h>
#include <ecl_rft_node.h>
#include <hash.h>
#include <vector.h>
#include <ecl_file.h>

struct ecl_rft_file_struct {
  char        * filename;
  vector_type * data;
};



static ecl_rft_file_type * ecl_rft_file_alloc_empty(const char * filename) {
  ecl_rft_file_type * rft_vector = util_malloc(sizeof * rft_vector , __func__);
  rft_vector->data      = vector_alloc_new();
  rft_vector->filename  = util_alloc_string_copy(filename);
  return rft_vector;
}



static void ecl_rft_file_add_node(ecl_rft_file_type * rft_vector , const ecl_rft_node_type * rft_node) {
  vector_append_owned_ref( rft_vector->data , rft_node , ecl_rft_node_free__);
}




ecl_rft_file_type * ecl_rft_file_alloc(const char * filename) {
  bool endian_flip;
  fortio_guess_endian_flip( filename , &endian_flip);
  {
    bool fmt_file                    = ecl_util_fmt_file(filename); 
    ecl_rft_file_type * rft_vector = ecl_rft_file_alloc_empty(filename);
    fortio_type   * fortio = fortio_fopen( filename , "r" , endian_flip , fmt_file);
    bool complete = false;
    do {
      ecl_file_type * ecl_file = ecl_file_fread_alloc_RFT_section( fortio );
      if (ecl_file != NULL) {
	ecl_rft_node_type * rft_node = ecl_rft_node_alloc( ecl_file );
	if (rft_node != NULL) 
	  ecl_rft_file_add_node(rft_vector , rft_node);
	ecl_file_free( ecl_file );
      } else complete = true;
    } while ( !complete );
    fortio_fclose( fortio );
    return rft_vector;
  }
}




void ecl_rft_file_free(ecl_rft_file_type * rft_vector) {
  vector_free(rft_vector->data);
  free(rft_vector->filename);
  free(rft_vector);
}


void ecl_rft_file_summarize(const ecl_rft_file_type * rft_vector , bool show_completions) {
  int iw;
  for (iw = 0; iw < vector_get_size( rft_vector->data ); iw++) {
    ecl_rft_node_summarize(vector_iget( rft_vector->data , iw) , show_completions);
    printf("\n");
  }
}

