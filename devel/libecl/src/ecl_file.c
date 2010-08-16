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
#include <stringlist.h>
#include <ecl_endian_flip.h>

/**
   This file implements functionality to load an entire ECLIPSE file
   in ecl_kw format. In addition to loading a complete file it can
   also load a section, by stopping when it meets a certain
   keyword. This latter functionality is suitable for loading parts
   (i.e. one report step) of unified files.

   The ecl_file struct is quite simply a vector of ecl_kw instances,
   it has no knowledge of report steps and such, and does not know
   whether it has been built from a complete file, or only from a part
   of a unified file.
*/





#define ECL_FILE_ID 776107


struct ecl_file_struct {
  UTIL_TYPE_ID_DECLARATION;
  char        	  * src_file;  	  /* The name of the file currently loaded - as returned from fortio. */
  vector_type 	  * kw_list;   	  /* This is a vector of ecl_kw instances corresponding to the content of the file. */
  hash_type   	  * kw_index;  	  /* A hash table with integer vectors of indices - see comment below. */
  stringlist_type * distinct_kw;  /* A stringlist of the keywords occuring in the file - each string occurs ONLY ONCE. */
};


/*
  This illustrates the indexing. The ecl_file instance contains in
  total 7 ecl_kw instances, the global index [0...6] is the internal
  way to access the various keywords. The kw_index is a hash table
  with entries 'SEQHDR', 'MINISTEP' and 'PARAMS'. Each entry in the
  hash table is an integer vector which again contains the internal
  index of the various occurences:
  
   ------------------
   SEQHDR            \
   MINISTEP  0        |     
   PARAMS    .....    |
   MINISTEP  1        |
   PARAMS    .....    |
   MINISTEP  2        |
   PARAMS    .....   /
   ------------------

   kw_index    = {"SEQHDR": [0], "MINISTEP": [1,3,5], "PARAMS": [2,4,6]}    <== This is hash table.
   kw_list     = [SEQHDR , MINISTEP , PARAMS , MINISTEP , PARAMS , MINISTEP , PARAMS]
   distinct_kw = [SEQHDR , MINISTEP , PARAMS]
   
*/

UTIL_SAFE_CAST_FUNCTION( ecl_file , ECL_FILE_ID)


static ecl_file_type * ecl_file_alloc_empty( ) {
  ecl_file_type * ecl_file = util_malloc( sizeof * ecl_file , __func__);
  UTIL_TYPE_ID_INIT(ecl_file , ECL_FILE_ID);
  ecl_file->kw_list  	= vector_alloc_new();
  ecl_file->kw_index 	= hash_alloc();
  ecl_file->src_file 	= NULL;
  ecl_file->distinct_kw = stringlist_alloc_new();
  return ecl_file;
}


/**
   This function iterates over the kw_list vector and builds the
   internal index fields 'kw_index' and 'distinct_kw'. This function
   must be called everytime the content of the kw_list vector is
   modified (otherwise the ecl_file instance will be in an
   inconsistent state).
*/

static void ecl_file_make_index( ecl_file_type * ecl_file ) {
  stringlist_clear( ecl_file->distinct_kw );
  hash_clear( ecl_file->kw_index );
  {
    int i;
    for (i=0; i < vector_get_size( ecl_file->kw_list ); i++) {
      const ecl_kw_type * ecl_kw = vector_iget_const( ecl_file->kw_list , i);
      char              * header = ecl_kw_alloc_strip_header( ecl_kw );
      if ( !hash_has_key( ecl_file->kw_index , header )) {
	int_vector_type * index_vector = int_vector_alloc( 0 , -1 );
	hash_insert_hash_owned_ref( ecl_file->kw_index , header , index_vector , int_vector_free__);
	stringlist_append_copy( ecl_file->distinct_kw , header);
      }
      
      {
	int_vector_type * index_vector = hash_get( ecl_file->kw_index , header);
	int_vector_append( index_vector , i);
      }
      free(header);
    }
  }
}



/**
   This function will seek for occurence nr 'occurence' of 'kw' in the
   fortio instance, and position the stream pointer there. If the
   occuerence is found, the function will return true, otherwise it
   will return false, and reposition the stream pointer at position it
   had prior to the call.

   Observe that the occurence variable is a 'normal' C-based index,
   i.e. to find the first occurence of e.g. 'PRESSURE' you issue the
   call:

       ecl_file_ifseek_kw(fortio , "PRESSURE" , 0);

   This function is used when loading report_step nr xxx from a
   unified summary file. (For unified restart files, better
   alternatives exist).  
*/


static bool ecl_file_ifseek_kw(fortio_type * fortio, const char * kw , int occurence) {
  FILE * stream    = fortio_get_FILE( fortio ); 
  bool pos_found   = false;
  bool cont        = true;
  int  find_count  = 0;
  long start_pos   = ftell( stream );
  long last_pos;
  ecl_kw_type * ecl_kw = ecl_kw_alloc_empty();
  do {
    last_pos = ftell( stream );
    
    if (ecl_kw_fread_header(ecl_kw , fortio)) {
      if (ecl_kw_header_eq(ecl_kw , kw)) {
	if (find_count == occurence) {
	  pos_found = true;
	  cont      = false;
	}
	find_count++;
      }
      /* Skip the data section of the keyword and continue. */
      ecl_kw_fskip_data( ecl_kw , fortio );
    } else
      cont = false;
  } while (cont);
  ecl_kw_free( ecl_kw );

  if (pos_found) 
    fseek( stream , last_pos , SEEK_SET);  /* Reposition to the top of the last keyword - that was the one we wanted. */
  else
    fseek( stream , start_pos , SEEK_SET); /* Reposition to the initial position and return false. */
  
  return pos_found;
}




/** 
   This function will allocate and read a ecl_file instance from an
   open fortio instance. If stop_kw != NULL the function will return
   __THE_SECOND_TIME__ stop_kw is encountered, leaving the fortio
   pointer looking at the stop_kw keyword. If stop_kw == NULL the
   function will read the complete file. The stop_kw parameter is used
   to support partial reading of unified files (RFT/SUMMARY/RESTART).

   If no keywords are read in the function will return NULL (and not
   an empty ecl_file skeleton).
*/


static ecl_file_type * ecl_file_fread_alloc_fortio(fortio_type * fortio , const char * stop_kw) {
  bool first_kw            = true;
  int stop_count           = 0;
  ecl_file_type * ecl_file = ecl_file_alloc_empty();
  ecl_kw_type   * ecl_kw;

  ecl_file->src_file = util_alloc_string_copy( fortio_filename_ref( fortio ) );
  do {
    ecl_kw = ecl_kw_fread_alloc( fortio );
    if (ecl_kw != NULL) {
      if (stop_kw != NULL) {
	bool eq = ecl_kw_header_eq( ecl_kw , stop_kw);
	
	if (first_kw) 
	  if (!eq)
	    util_abort("%s: expected to find:%s as first keyword - aborting \n",__func__ , stop_kw);
	
	if (eq)
	  stop_count++;
	
	if (stop_count == 2) { /* Two strikes and you are out ... */
	  ecl_kw_rewind(ecl_kw , fortio);
	  ecl_kw_free( ecl_kw );
	  ecl_kw = NULL;
	}
      }
    }
    
    /* 
       Must have a new check on NULL - because the ecl_kw instance can
       be freed and set to to NULL in the preceeding block. 
    */
    if (ecl_kw != NULL) 
      vector_append_owned_ref( ecl_file->kw_list , ecl_kw , ecl_kw_free__);
    
    first_kw = false;
  } while (ecl_kw != NULL);

  
  
  /* Returning NULL for an empty file. */
  if (vector_get_size(ecl_file->kw_list) == 0) {
    ecl_file_free( ecl_file );
    ecl_file = NULL;
  } else
    /* Building up the index for keyword/occurence based lookup. */
    ecl_file_make_index( ecl_file );
  
  return ecl_file;
}






/*
  This is the generic - "load a file" function. It will take ANY
  ECLIPSE file with ecl_kw format and internalize all the keywords as
  ecl_kw instances in one long vector.
*/

ecl_file_type * ecl_file_fread_alloc(const char * filename ) {
  bool          fmt_file   = ecl_util_fmt_file( filename );
  fortio_type * fortio     = fortio_fopen(filename , "r" , ECL_ENDIAN_FLIP , fmt_file);
  
  ecl_file_type * ecl_file = ecl_file_fread_alloc_fortio(fortio , NULL);
    
  fortio_fclose( fortio );
  return ecl_file;
}



/* 
   This function will allocate a ecl_file_type instance going all the
   way to the NEXT 'SEQHDR' keyword. Observe that it is assumed that
   the fortio instance is already positioned at a SEQHDR keyword.
   
   Will return NULL if the fortio pointer is already at the end of
   the file.
*/


ecl_file_type * ecl_file_fread_alloc_summary_section(fortio_type * fortio) {
  ecl_file_type * summary_section = ecl_file_fread_alloc_fortio(fortio , "SEQHDR");
  return summary_section;
}


/**
   This file will read and allocate section (i.e. corresponding to one
   report step), for a unified summary file. If you are going to
   allocate the whole damned file, you are better off with using
   ecl_file_fread_alloc_summary_section().

   Observe that there is some counting-fuckup here: The libecl library
   generally follows C conventions, with all counters starting at
   zero. However the first report_step in a summary file has (by
   defintion number 1, i.e. the first summary file is ECLIPSE.S0001),
   hence this function assumes index to be 1 offset (Ohhh this is so
   ugly), and then shift it before going further.
   
   If the occurence you are asking for can not be found the whole
   function will return NULL - calling scope has to check this.

   The functions ecl_file_fread_alloc_unsmry_section() and
   ecl_file_fread_alloc_summary_section() can be considered a pair,
   where the first is the high level function operating with a
   filename, and the second (which actually does the work) operates on
   a fortio instance.
*/


ecl_file_type * ecl_file_fread_alloc_unsmry_section(const char * filename , int index) {
  bool          fmt_file   = ecl_util_fmt_file( filename );
  fortio_type * fortio     = fortio_fopen(filename , "r" , ECL_ENDIAN_FLIP , fmt_file);
  ecl_file_type * ecl_file = NULL;

  if (ecl_file_ifseek_kw( fortio , "SEQHDR" , index - 1)) 
    ecl_file = ecl_file_fread_alloc_summary_section(fortio);
  else
    util_abort("%s: sorry - could not lcoate summary report:%d in file:%s \n",__func__ , index , filename);
  
  fortio_fclose( fortio );
  
  return ecl_file;
}




/**
   The SEQNUM number found in unified restart files corresponds to the 
   REPORT_STEP.
*/
ecl_file_type * ecl_file_fread_alloc_restart_section(fortio_type * fortio) {
  return ecl_file_fread_alloc_fortio(fortio , "SEQNUM");
}




/**
   This function will look up the INTEHEAD keyword in a ecl_file_type
   instance, and calculate simulation date from this instance.

   Will fail hard if it is impossible to find the queried for INTEHEAD
   occurence.
*/



static time_t ecl_file_iget_restart_sim_date__(const ecl_file_type * restart_file , int occurence) {
  ecl_kw_type * intehead_kw = ecl_file_iget_named_kw( restart_file , "INTEHEAD" , occurence );
  return util_make_date( ecl_kw_iget_int( intehead_kw , 64) , ecl_kw_iget_int( intehead_kw , 65) , ecl_kw_iget_int( intehead_kw , 66));
}


time_t ecl_file_iget_restart_sim_date( const ecl_file_type * restart_file , int occurence ) {
  return ecl_file_iget_restart_sim_date__( restart_file , occurence );
}



/**
   Will look through the unified restart file and load the section
   corresponding to report_step 'report_step'. If the report_step can
   not be found the function will return NULL.

   The ecl_file_fread_alloc_unrst_section() function positions the
   fortio pointer correctly in the file, and then calls the
   ecl_file_fread_alloc_restart_section() function which does the
   actual loading.
*/

ecl_file_type * ecl_file_fread_alloc_unrst_section(const char * filename , int report_step) {
  ecl_kw_type * seqnum_kw  = ecl_kw_alloc_new( "SEQNUM" , 1 , ECL_INT_TYPE , &report_step);  
                             /* We will use ecl_kw_equal() based on this kw to find the correct location in the file. */  
  bool          fmt_file   = ecl_util_fmt_file( filename );
  fortio_type * fortio     = fortio_fopen(filename , "r" , ECL_ENDIAN_FLIP , fmt_file);
  FILE * stream            = fortio_get_FILE( fortio );
  ecl_file_type * ecl_file = NULL;
  long read_pos            = 0;
  bool section_found       = false;
  bool cont                = true;
  ecl_kw_type * file_kw    = ecl_kw_alloc_empty();
  do {
    if (ecl_kw_fseek_kw("SEQNUM" , false, false , fortio)) {
      read_pos              = ftell( stream );
      if (ecl_kw_fread_header( file_kw , fortio )) {
	if (ecl_kw_header_eq( file_kw , "SEQNUM")) {
	  ecl_kw_alloc_data( file_kw );  /* If we have the right header we continue to read in data. */
	  ecl_kw_fread_data( file_kw , fortio );
	  if (ecl_kw_equal( file_kw , seqnum_kw )) {
	    section_found = true;
	    cont          = false;
	  }
	  ecl_kw_free_data( file_kw ); /* Discard the data */
	} else
	  ecl_kw_fskip_data( file_kw , fortio );
      } else
	cont = false;
    } else cont = false;
  } while (cont);
  ecl_kw_free( file_kw );

  if (section_found) {
    fseek(stream , read_pos , SEEK_SET);
    ecl_file = ecl_file_fread_alloc_restart_section( fortio );
  } 
  fortio_fclose( fortio );
  ecl_kw_free( seqnum_kw );
  
  return ecl_file;
}


ecl_file_type * ecl_file_fread_alloc_RFT_section(fortio_type * fortio) {
  return ecl_file_fread_alloc_fortio(fortio , "TIME");
}


/**
   This function will insert a new ecl_kw instance in the
   ecl_file. The ecl_file instance will take ownership of the new
   instance, and free it when the ecl_file is freed.
   
   Where is the new ecl_kw inserted??

    1. The position indicated by (neighbour_name , neighbour_occurence)
       is located.

    2. If the boolean 'after' is true the new ecl_kw is placed
       immediately after the location found in point 1, otherwise it is
       placed immediately before.

    3. neighbour_name can be NULL, in which case the new ecl_kw is
       placed at the end (after == true), or beginning of the ecl_file
       (after == false); in this case neighbour_occurence is not
       considered.

    4. If the position (neighbour_name , neighbour_occurence) can not
       be found the function will fail hard - check first with
       ecl_file_get_num_named_kw() first.


   So, considering the example at the top of the file, the following
   statements:
   
     ecl_file_insert_kw( ecl_file , FUNNY_KW  , true , "MINISTEP" , 2);
     ecl_file_insert_kw( ecl_file , NEW_START , false , NULL , 0);

   Will result in the ecl_file instance:
   
   ------------------
   NEW_START         \      <- Newly inserted 
   SEQHDR             |
   MINISTEP  0        |     
   PARAMS    .....    |
   MINISTEP  1        |
   PARAMS    .....    |
   MINISTEP  2        |
   FUNNY_KW           |     <- Newly inserted
   PARAMS    .....   /
   ------------------
*/

void ecl_file_insert_kw( ecl_file_type * ecl_file , ecl_kw_type * ecl_kw , bool after , const char * neighbour_name , int neighbour_occurence ) {
  if (neighbour_name == NULL) {
    if (after)
      vector_append_owned_ref( ecl_file->kw_list , ecl_kw , ecl_kw_free__ );
    else
      vector_insert_owned_ref( ecl_file->kw_list , 0 , ecl_kw , ecl_kw_free__ );
  } else {
    const int_vector_type * index_vector = hash_get(ecl_file->kw_index , neighbour_name);
    int global_index = int_vector_iget( index_vector , neighbour_occurence);

    if (after)
      global_index++;
    
    vector_insert_owned_ref( ecl_file->kw_list , global_index , ecl_kw , ecl_kw_free__);
  }
  ecl_file_make_index( ecl_file );
}



/**
   This function will delete the keyword (name,occurence) from the
   ecl_file instance. When deleted from the ecl_file instance the
   ecl_kw instance will be freed.

   If the keyword corresponding to (name,occurence) can not be found the
   function will fail hard.
*/
   
void ecl_file_delete_kw( ecl_file_type * ecl_file , ecl_kw_type * ecl_kw , bool after , const char * name , int occurence ) {
  const int_vector_type * index_vector = hash_get(ecl_file->kw_index , name);
  int global_index = int_vector_iget( index_vector , occurence);
  vector_idel( ecl_file->kw_list , global_index );
  ecl_file_make_index( ecl_file );
}


/*****************************************************************/
/* fwrite functions */

void ecl_file_fwrite_fortio(const ecl_file_type * ecl_file , fortio_type * fortio, int offset) {
  int index;
  for (index = offset; index < vector_get_size( ecl_file->kw_list ); index++)
    ecl_kw_fwrite( vector_iget( ecl_file->kw_list , index ) , fortio);
}



/* 
   Observe : if the filename is a standard filename which can be used
   to infer formatted/unformatted automagically the fmt_file variable
   is NOT consulted.
*/
  
void ecl_file_fwrite(const ecl_file_type * ecl_file , const char * filename, bool fmt_file) {
  bool __fmt_file;
  ecl_file_enum file_type;
  
  file_type = ecl_util_get_file_type( filename , &__fmt_file , NULL);
  if (file_type == ECL_OTHER_FILE)
    __fmt_file = fmt_file;
  
  {
    fortio_type * fortio = fortio_fopen( filename , "w", ECL_ENDIAN_FLIP , __fmt_file);
    ecl_file_fwrite_fortio( ecl_file , fortio , 0);
    fortio_fclose( fortio );
  }
}




void ecl_file_free(ecl_file_type * ecl_file) {
  vector_free( ecl_file->kw_list );
  hash_free( ecl_file->kw_index );
  util_safe_free( ecl_file->src_file );
  stringlist_free( ecl_file->distinct_kw );
  free(ecl_file);
}


void ecl_file_free__(void * arg) {
  ecl_file_free( ecl_file_safe_cast( arg ) );
}



/*****************************************************************/
/**
  Here comes several functions for querying the ecl_file instance, and
  getting pointers to the ecl_kw content of the ecl_file. For getting
  ecl_kw instances there are two principally different access methods:

    * ecl_file_iget_named_kw(): This function will take a keyword
      (char *) and an integer as input. The integer corresponds to the
      ith occurence of the keyword in the file.

    * ecl_file_iget_kw(): This function just takes an integer index as
      input, and returns the corresponding ecl_kw instance - without
      considering which keyword it is.

  -------

  In addition the functions ecl_file_get_num_distinct_kw() and
  ecl_file_iget_distinct_kw() will return the number of distinct
  keywords, and distinct keyword keyword nr i (as a const char *).


  Possible usage pattern:

  ....
  for (ikw = 0; ikw < ecl_file_get_num_distinct_kw(ecl_file); ikw++) {
     const char * kw = ecl_file_iget_distinct_kw(ecl_file , ikw);
     
     printf("The file contains: %d occurences of \'%s\' \n",ecl_file_get_num_named_kw( ecl_file , kw) , kw);
  }
  ....

  For the summary file showed in the top this code will produce:

    The file contains 1 occurences of 'SEQHDR'
    The file contains 3 occurences of 'MINISTEP'
    The file contains 3 occurences of 'PARAMS'
  
*/




/**
   This will just return ecl_kw nr i - without looking at the names.
*/
ecl_kw_type * ecl_file_iget_kw( const ecl_file_type * ecl_file , int index) {
  return vector_iget( ecl_file->kw_list , index);
}



/**
   This will return a copy ecl_kw nr i - without looking at the names.
*/
ecl_kw_type * ecl_file_icopy_kw( const ecl_file_type * ecl_file , int index) {
  return ecl_kw_alloc_copy( vector_iget( ecl_file->kw_list , index) );
}



/* 
   This function will return the ith occurence of 'kw' in
   ecl_file. Will abort hard if the request can not be satisifed - use
   query functions if you can not take that.
*/
   

ecl_kw_type * ecl_file_iget_named_kw( const ecl_file_type * ecl_file , const char * kw, int ith) {
  const int_vector_type * index_vector = hash_get(ecl_file->kw_index , kw);
  int global_index = int_vector_iget( index_vector , ith);
  return ecl_file_iget_kw( ecl_file , global_index );
}


ecl_kw_type * ecl_file_icopy_named_kw( const ecl_file_type * ecl_file , const char * kw, int ith) {
  return ecl_kw_alloc_copy( ecl_file_iget_named_kw( ecl_file , kw , ith ));
}

  
/*
  Will return the number of times a particular keyword occurs in a
  ecl_file instance. Will return 0 if the keyword can not be found.
*/

int ecl_file_get_num_named_kw(const ecl_file_type * ecl_file , const char * kw) {
  if (hash_has_key(ecl_file->kw_index , kw)) {
    const int_vector_type * index_vector = hash_get(ecl_file->kw_index , kw);
    return int_vector_size( index_vector );
  } else
    return 0;
}




/**
   This function does the following:

    1. Takes an input index which goes in to the global kw_list vector.
    2. Looks up the corresponding keyword.
    3. Return the number of this particular keyword instance, among
       the other instance with the same header.

   With the example above we get:

     ecl_file_iget_occurence(ecl_file , 2) -> 0; Global index 2 will
        look up the first occurence of PARAMS.
  
     ecl_file_iget_occurence(ecl_file , 5) -> 2; Global index 5 will
        look up th third occurence of MINISTEP.

   The enkf layer uses this funny functionality.
*/


int ecl_file_iget_occurence( const ecl_file_type * ecl_file , int index) {
  const ecl_kw_type * ecl_kw = ecl_file_iget_kw( ecl_file , index );
  char * header = ecl_kw_alloc_strip_header( ecl_kw );
  const int_vector_type * index_vector = hash_get(ecl_file->kw_index , header );
  const int * index_data = int_vector_get_const_ptr( index_vector );
  
  int occurence = -1;
  {
    /* Manual reverse lookup. */
    for (int i=0; i < int_vector_size( index_vector ); i++)
      if (index_data[i] == index)
	occurence = i;
  }
  if (occurence < 0)
    util_abort("%s: internal error ... \n" , __func__);

  free(header);
  return occurence;
}


/** 
    Returns the total number of ecl_kw instances in the ecl_file
    instance.
*/
int ecl_file_get_num_kw( const ecl_file_type * ecl_file ){
  return vector_get_size( ecl_file->kw_list );
}


/**
   Returns true if the ecl_file instance has at-least one occurence of
   ecl_kw 'kw'.
*/
bool ecl_file_has_kw( const ecl_file_type * ecl_file , const char * kw) {
  return hash_has_key( ecl_file->kw_index , kw );
}


int ecl_file_get_num_distinct_kw(const ecl_file_type * ecl_file) {
  return stringlist_get_size( ecl_file->distinct_kw );
}


const char * ecl_file_iget_distinct_kw(const ecl_file_type * ecl_file, int index) {
  return stringlist_iget( ecl_file->distinct_kw , index);
}


