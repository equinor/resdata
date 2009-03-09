#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fortio.h>
#include <ecl_kw.h>
#include <ecl_file.h>
#include <errno.h>
#include <hash.h>
#include <util.h>
#include <time.h>
#include <vector.h>
#include <int_vector.h>


#define ECL_FILE_ID 776107



struct ecl_file_struct {
  int           __id;
  char        * src_file;  /* The name of the file currently loaded - as returned from fortio. */
  vector_type * kw_list;   /* This is a vector of ecl_kw instances corresponding to the content of the file. */
  hash_type   * kw_index;  /* A hash table with integer vectors of indices - see comment below. */
};


/*
   ------------------
   SEQHDR            \
   MINISTEP  0        |     
   PARAMS    .....    |
   MINISTEP  1        |
   PARAMS    .....    |
   MINISTEP  2        |
   PARAMS    .....   /
   ------------------


   kw_index = {"SEQHDR": <0>, "MINISTEP": <1,3,5>, "PARAMS": <2,4,6>}
   
*/



static ecl_file_type * ecl_file_safe_cast( void * arg) {
  ecl_file_type * ecl_file = (ecl_file_type * ) arg;

  if (ecl_file->__id != ECL_FILE_ID) {
    util_abort("%s: run_time cast failed - aborting \n",__func__);
    ecl_file = NULL;
  }

  return ecl_file;
}
  



ecl_file_type * ecl_file_alloc_empty( ) {
  ecl_file_type * ecl_file = util_malloc( sizeof * ecl_file , __func__);
  ecl_file->kw_list  = vector_alloc_new();
  ecl_file->__id     = ECL_FILE_ID;
  ecl_file->kw_index = hash_alloc();
  ecl_file->src_file = NULL;
  return ecl_file;
}





/* 
   This function will allocate and read a ecl_file instance from an
   open fortio instance. If stop_kw != NULL the function will return
   __THE_SECOND_TIME__ stop_kw is encountered, leaving the fortio
   pointer looking at the stop_kw keyword. If stop_kw == NULL the
   function will read the complete file.

   If no keywords are read in the function will return NULL (and not
   an empty ecl_file skeleton).
*/


static ecl_file_type * ecl_file_fread_alloc_fortio(fortio_type * fortio , const char * stop_kw) {
  int stop_count           = 0;
  ecl_file_type * ecl_file = ecl_file_alloc_empty();
  ecl_kw_type   * ecl_kw;
  do {
    ecl_kw = ecl_kw_fread_alloc( fortio );
    if (ecl_kw != NULL) {
      if (stop_kw != NULL) {
	if (ecl_kw_header_eq( ecl_kw , stop_kw))
	  stop_count++;
	
	if (stop_count == 2) { /* Two strikes and you are out ... */
	  ecl_kw_rewind(ecl_kw , fortio);
	  ecl_kw_free( ecl_kw );
	  ecl_kw = NULL;
	}
      }
    }
    if (ecl_kw != NULL) {
      int index = vector_append_owned_ref( ecl_file->kw_list , ecl_kw , ecl_kw_free__);
      char * header = ecl_kw_alloc_strip_header( ecl_kw );
      if (! hash_has_key( ecl_file->kw_index , header )) 
	hash_insert_hash_owned_ref( ecl_file->kw_index , header , int_vector_alloc(0 , -1) , int_vector_free__);
      {
	int_vector_type * index_vector = hash_get( ecl_file->kw_index , header);
	int_vector_append( index_vector , index);
      }
      free(header);
    }
  } while (ecl_kw != NULL);
  
  
  /* Returning NULL for an empty file. */
  if (vector_get_size(ecl_file->kw_list) == 0) {
    ecl_file_free( ecl_file );
    ecl_file = NULL;
  }

  ecl_file->src_file = util_alloc_string_copy( fortio_filename_ref( fortio ) );
  return ecl_file;
}






/*
  This is the generic - "load a file" function. It will take ANY
  ECLIPSE file and internalize all the keywords as ecl_kw instances in
  one long vector.
*/

ecl_file_type * ecl_file_fread_alloc(const char * filename , bool endian_flip) {
  bool          fmt_file   = ecl_util_fmt_file( filename );
  fortio_type * fortio     = fortio_fopen(filename , "r" , endian_flip , fmt_file);
  
  ecl_file_type * ecl_file = ecl_file_fread_alloc_fortio(fortio , NULL);
    
  fortio_fclose( fortio );
  return ecl_file;
}



/* 
   This function will allocate a ecl_file_type instance going all the
   way to the next 'SEQHDR' keyword.
*/


ecl_file_type * ecl_file_alloc_summary_section(fortio_type * fortio) {
  return ecl_file_fread_alloc_fortio(fortio , "SEQHDR");
}


ecl_file_type * ecl_file_alloc_restart_section(fortio_type * fortio) {
  return ecl_file_fread_alloc_fortio(fortio , "SEQNUM");
}


void ecl_file_free(ecl_file_type * ecl_file) {
  vector_free( ecl_file->kw_list );
  hash_free( ecl_file->kw_index );
  util_safe_free( ecl_file->src_file );
  free(ecl_file);
}


void ecl_file_free__(void * arg) {
  ecl_file_free( ecl_file_safe_cast( arg ) );
}


/*****************************************************************/


/* 
   This function will return the ith occurence of 'kw' in
   ecl_file. Will abort hard if the request can not be satisifed - use
   query functions if you can not take that.
*/
   

ecl_kw_type * ecl_file_iget_kw( const ecl_file_type * ecl_file , const char * kw, int ith) {
  const int_vector_type * index_vector = hash_get(ecl_file->kw_index , kw);
  int global_index = int_vector_iget( index_vector , ith);
  return vector_iget( ecl_file->kw_list , global_index );
}

