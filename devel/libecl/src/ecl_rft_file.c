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
#include <int_vector.h>

/**
   This data structure is for loading one eclipse RFT file. One RFT
   file can in general contain RFT information from:

     o Many different times
     o Many different wells

   All of this is just lumped together in one long vector, both in the
   file, and in this implementation. The data for one specific RFT
   (one well, one time) is internalized in the ecl_rft_node type.   
   
   
*/
   


struct ecl_rft_file_struct {
  char        * filename;
  vector_type * data;          /* This vector just contains all the rft nodes in one long vector. */
  hash_type   * well_index;    /* This indexes well names into the data vector. */
};



static ecl_rft_file_type * ecl_rft_file_alloc_empty(const char * filename) {
  ecl_rft_file_type * rft_vector = util_malloc(sizeof * rft_vector , __func__);
  rft_vector->data       = vector_alloc_new();
  rft_vector->filename   = util_alloc_string_copy(filename);
  rft_vector->well_index = hash_alloc();
  return rft_vector;
}



static void ecl_rft_file_add_node(ecl_rft_file_type * rft_vector , const ecl_rft_node_type * rft_node) {
  vector_append_owned_ref( rft_vector->data , rft_node , ecl_rft_node_free__);
}




ecl_rft_file_type * ecl_rft_file_alloc(const char * filename) {
  bool endian_flip;
  fortio_guess_endian_flip( filename , &endian_flip);
  {
    bool fmt_file                  = ecl_util_fmt_file(filename); 
    ecl_rft_file_type * rft_vector = ecl_rft_file_alloc_empty(filename);
    fortio_type   * fortio         = fortio_fopen( filename , "r" , endian_flip , fmt_file);
    bool complete = false;
    int global_index = 0;
    do {
      ecl_file_type * ecl_file = ecl_file_fread_alloc_RFT_section( fortio );
      if (ecl_file != NULL) {
	ecl_rft_node_type * rft_node = ecl_rft_node_alloc( ecl_file );
	if (rft_node != NULL) { 
	  const char * well_name = ecl_rft_node_get_well_name( rft_node );
	  ecl_rft_file_add_node(rft_vector , rft_node);
	  if (!hash_has_key( rft_vector->well_index , well_name)) 
	    hash_insert_hash_owned_ref( rft_vector->well_index , well_name , int_vector_alloc(3 , 0) , int_vector_free__);
	  {
	    int_vector_type * index_list = hash_get( rft_vector->well_index , well_name);
	    int_vector_append(index_list , global_index);
	  }	  
	  global_index++;
	}
	ecl_file_free( ecl_file );
      } else complete = true;
    } while ( !complete );
    fortio_fclose( fortio );
    return rft_vector;
  }
}


void ecl_rft_file_free(ecl_rft_file_type * rft_vector) {
  vector_free(rft_vector->data);
  hash_free( rft_vector->well_index );
  free(rft_vector->filename);
  free(rft_vector);
}


/**
   Returns the total number of rft nodes in the file, not caring if
   the same well occurse many times and so on.
*/

int ecl_rft_file_get_size( const ecl_rft_file_type * rft_file) {
  return vector_get_size( rft_file->data );
}



/**
   Return rft_node number 'i' in the rft_file - not caring when this
   particular RFT is from, or which well it is. 

   If you ask for an index which is beyond the size of the vector it will
   go up in flames - use ecl_file_get_size() first if you can not
   handle that.
*/

const ecl_rft_node_type * ecl_rft_file_iget_node( const ecl_rft_file_type * rft_file , int index) {
  return vector_iget_const( rft_file->data , index );
}



/**
   This function will return ecl_rft_node nr index - for well
   'well'. I.e. for an RFT file which looks like this:

   RFT - Well P1: 01/01/2000
   RFT - Well P2: 01/01/2000
   RFT - WEll P1: 01/01/2001
   RFT - Well P2: 01/01/2001   <--
   RFT - Well P1: 01/01/2002
   RFT - Well P2: 01/01/2002

   The function call: 

      ecl_rft_iget_well_rft(rft_file , "P2" , 1) 

   will return the rft node indicated by the arrow (i.e. the second
   occurence of well "P2" in the file.)

   If the rft_file does not have the well, or that occurence the
   function will go down in flames with util_abort(). Use
   ecl_rft_file_has_well() and ecl_rft_file_get_well_occurences()
   first if you can not take util_abort().
*/



const ecl_rft_node_type * ecl_rft_file_iget_well_rft( const ecl_rft_file_type * rft_file , const char * well, int index) {
  const int_vector_type * index_vector = hash_get(rft_file->well_index , well);
  return ecl_rft_file_iget_node( rft_file , int_vector_iget(index_vector , index));
}


bool ecl_rft_file_has_well( const ecl_rft_file_type * rft_file , const char * well) {
  return hash_has_key(rft_file->well_index , well);
}


/**
   Returns the number of occurences of 'well' in rft_file.
*/

int ecl_rft_file_get_well_occurences( const ecl_rft_file_type * rft_file , const char * well) {
  const int_vector_type * index_vector = hash_get(rft_file->well_index , well);
  return int_vector_size( index_vector );
}


/** 
   Returns the number of distinct wells in RFT file.
*/
int ecl_rft_file_get_num_wells( const ecl_rft_file_type * rft_file ) {
  return hash_get_size( rft_file->well_index );
}



stringlist_type * ecl_rft_file_alloc_well_list(const ecl_rft_file_type * rft_file ) {
  return hash_alloc_stringlist( rft_file->well_index );
}


/*****************************************************************/


void ecl_rft_file_summarize(const ecl_rft_file_type * rft_vector , bool show_completions) {
  int iw;
  for (iw = 0; iw < vector_get_size( rft_vector->data ); iw++) {
    ecl_rft_node_summarize(vector_iget( rft_vector->data , iw) , show_completions);
    printf("\n");
  }
}

