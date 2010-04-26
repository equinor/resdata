#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <util.h>
#include <stringlist.h>
#include <vector.h>
#include <buffer.h>


#define STRINGLIST_TYPE_ID 671855

/**
   This file implements a very thin wrapper around a list (vector) of
   strings, and the total number of strings. It is mostly to avoid
   sending both argc and argv.
   
   Most of the functionality is implemented through vector.c and
   stateless functions in util.c
*/


struct stringlist_struct {
  UTIL_TYPE_ID_DECLARATION;
  vector_type * strings;
};




static void stringlist_fprintf__(const stringlist_type * stringlist, const char * sep , FILE * stream) {
  int i;
  for (i=0; i < vector_get_size( stringlist->strings ); i++) {
    const char * s = stringlist_iget(stringlist , i);
    fprintf(stream , "%s%s", s  , sep);
  }
}


void stringlist_fprintf(const stringlist_type * stringlist, const char * sep , FILE * stream) {
  stringlist_fprintf__(stringlist , sep , stream);
}






/**
   This function appends a copy of s into the stringlist.
*/
void stringlist_append_copy(stringlist_type * stringlist , const char * s) {
  vector_append_buffer(stringlist->strings , s , strlen(s) + 1);
}

void stringlist_append_ref(stringlist_type * stringlist , const char * s) {
  vector_append_ref(stringlist->strings , s);
}

void stringlist_append_owned_ref(stringlist_type * stringlist , const char * s) {
  vector_append_owned_ref(stringlist->strings , s , free);
}

/*****************************************************************/

void stringlist_iset_copy(stringlist_type * stringlist , int index , const char * s) {
  vector_iset_buffer(stringlist->strings , index , s , strlen(s) + 1);
}

void stringlist_iset_ref(stringlist_type * stringlist , int index , const char * s) {
  vector_iset_ref(stringlist->strings , index , s);
}

void stringlist_iset_owned_ref(stringlist_type * stringlist , int index , const char * s) {
  vector_iset_owned_ref(stringlist->strings , index , s , free);
}

/*****************************************************************/

void stringlist_insert_copy(stringlist_type * stringlist , int index , const char * s) {
  vector_insert_buffer(stringlist->strings , index , s , strlen(s) + 1);
}

void stringlist_insert_ref(stringlist_type * stringlist , int index , const char * s) {
  vector_insert_ref(stringlist->strings , index , s);
}

void stringlist_insert_owned_ref(stringlist_type * stringlist , int index , const char * s) {
  vector_insert_owned_ref(stringlist->strings , index , s , free);
}




static stringlist_type * stringlist_alloc_empty( bool alloc_vector ) {
  stringlist_type * stringlist = util_malloc(sizeof * stringlist , __func__);
  UTIL_TYPE_ID_INIT( stringlist , STRINGLIST_TYPE_ID);

  if (alloc_vector)
    stringlist->strings = vector_alloc_new();
  else
    stringlist->strings = NULL;
  
  return stringlist;
}




stringlist_type * stringlist_alloc_new() {
  return stringlist_alloc_empty( true );
}



stringlist_type * stringlist_alloc_argv_copy(const char ** argv , int argc) {
  int iarg;
  stringlist_type * stringlist = stringlist_alloc_empty( true);
  for (iarg = 0; iarg < argc; iarg++) 
    stringlist_append_copy( stringlist , argv[iarg]);
  
  return stringlist;
}


stringlist_type * stringlist_alloc_argv_ref(const char ** argv , int argc) {
  int iarg;
  stringlist_type * stringlist = stringlist_alloc_empty( true );
  for (iarg = 0; iarg < argc; iarg++) 
    stringlist_append_ref( stringlist , argv[iarg]);

  return stringlist;
}


stringlist_type * stringlist_alloc_argv_owned_ref(const char ** argv , int argc) {
  int iarg;
  stringlist_type * stringlist = stringlist_alloc_empty( true );
  for (iarg = 0; iarg < argc; iarg++) 
    stringlist_append_owned_ref( stringlist , argv[iarg]);
  
  return stringlist;
}



/** 
    Allocates a new stringlist instance where all the new string are references to the
    string found in the existing stringlist instance.
*/ 
stringlist_type * stringlist_alloc_shallow_copy(const stringlist_type * src) {
  stringlist_type * copy = stringlist_alloc_empty( false );
  copy->strings = vector_alloc_copy( src->strings , false);
  return copy;
}


/**
  Allocates a new stringlist where the strings are references to the
  num_strings found in stringlist from start.
*/
stringlist_type * stringlist_alloc_shallow_copy_with_limits(const stringlist_type * stringlist, int start, int num_strings) {
  stringlist_type * copy = stringlist_alloc_empty( true );

  for(int i=0; i<num_strings; i++)
  {
    const char * str = stringlist_iget(stringlist, i + start);
    vector_append_ref(copy->strings, str);
  }

  return copy;
}


/*
  Can not use vector_deep copy - because we might not have the
  constructor registered, in the node_data instance; but in this case
  we know the copy constructor.
*/

stringlist_type * stringlist_alloc_deep_copy(const stringlist_type * src) {
  stringlist_type * copy = stringlist_alloc_empty( true );
  int i;
  for (i = 0; i < stringlist_get_size( src ); i++) 
    stringlist_append_copy( copy , stringlist_iget( src , i));
  return copy;
}



void stringlist_append_stringlist_copy(stringlist_type * stringlist , const stringlist_type * src) {
  int i;
  for (i = 0; i < stringlist_get_size( src ); i++)
    stringlist_append_copy(stringlist , stringlist_iget(src , i));
}


void stringlist_append_stringlist_ref(stringlist_type * stringlist , const stringlist_type * src) {
  int i;
  for (i = 0; i < stringlist_get_size( src ); i++)
    stringlist_append_ref(stringlist , stringlist_iget(src , i));
}


/**
  Insert a copy of a stringlist in some position.
  
  Can probably be made more efficient.
*/

void stringlist_insert_stringlist_copy(stringlist_type * stringlist, const stringlist_type * src, int pos) {
  int size_old  = stringlist_get_size(stringlist);

  /** Cannot use assert_index here. */
  if(pos < 0 || pos > size_old)
    util_abort("%s: Position %d is out of bounds. Min: 0 Max: %d\n", pos, size_old);

  stringlist_type * start = stringlist_alloc_new();
  stringlist_type * end   = stringlist_alloc_new();
  stringlist_type * new   = stringlist_alloc_new();

  for(int i=0; i<pos; i++)
    stringlist_append_ref(start, stringlist_iget(stringlist, i));

  for(int i=pos; i<size_old; i++)
    stringlist_append_ref(end  , stringlist_iget(stringlist, i));

  stringlist_append_stringlist_copy(new, start);
  stringlist_append_stringlist_copy(new, src  );
  stringlist_append_stringlist_copy(new, end  );

  stringlist_clear(stringlist);
  stringlist_append_stringlist_copy(stringlist, new);

  stringlist_free(new);
  stringlist_free(start);
  stringlist_free(end);
}


/** 
    Frees all the memory contained by the stringlist.
*/
void stringlist_clear(stringlist_type * stringlist) {
  vector_clear( stringlist->strings );
}


void stringlist_free(stringlist_type * stringlist) {
  stringlist_clear(stringlist);
  vector_free(stringlist->strings);
  free(stringlist);
}


static UTIL_SAFE_CAST_FUNCTION(stringlist , STRINGLIST_TYPE_ID);
       UTIL_IS_INSTANCE_FUNCTION(stringlist , STRINGLIST_TYPE_ID)

void stringlist_free__(void * __stringlist) {
  stringlist_type * stringlist = stringlist_safe_cast(__stringlist);
  stringlist_free( stringlist );
}


void stringlist_idel(stringlist_type * stringlist , int index) {
  vector_idel( stringlist->strings , index);
}


const char * stringlist_iget(const stringlist_type * stringlist , int index) {
  return vector_iget(stringlist->strings ,index);
}


bool stringlist_iequal( const stringlist_type * stringlist , int index, const char * s ) {
  return util_string_equal( stringlist_iget( stringlist , index ) , s);
}


/**
   Will return NULL if you ask for something beyond the limits of the
   stringlist (will die on negative index - that is NEVER OK).
*/
const char * stringlist_safe_iget( const stringlist_type * stringlist , int index) {
  if (index < 0)
    util_abort("%s: negative index:%d is NOT allowed \n",__func__ , index);

  if (index < stringlist_get_size( stringlist ))
    return stringlist_iget( stringlist , index);
  else
    return NULL;
}



char * stringlist_iget_copy(const stringlist_type * stringlist , int index) {
  return util_alloc_string_copy(stringlist_iget(stringlist , index));
}


int stringlist_get_size(const stringlist_type * stringlist) {
  return vector_get_size(stringlist->strings);
}


/*
  Return NULL if the list has zero entries. 
*/
char ** stringlist_alloc_char_copy(const stringlist_type * stringlist) {
  char ** strings = NULL;
  int size = stringlist_get_size( stringlist );
  if (size > 0) {
    strings = util_malloc(size * sizeof * strings , __func__);
    for (int i = 0; i <size; i++)
      strings[i] = stringlist_iget_copy( stringlist , i);
  }
  return strings;
}





/** 
    Scans the stringlist (linear scan) to see if it contains (at
    least) one occurence of 's';
*/

bool stringlist_contains(const stringlist_type * stringlist , const char * s) {
  int  size     = stringlist_get_size( stringlist );
  int  index    = 0;
  bool contains = false;
  
  while ((index < size) && (!contains)) {
    const char * istring = stringlist_iget(stringlist , index);
    if (istring != NULL)
      if (strcmp(istring , s) == 0) contains = true;
    index++;
  }
  return contains;
}



/**
  Finds the indicies of the entries matching 's'.
*/
int_vector_type * stringlist_find(const stringlist_type * stringlist, const char * s) {
  int_vector_type * indicies = int_vector_alloc(0, -1);
  int  size     = stringlist_get_size( stringlist );
  int  index    = 0;
  
  while (index < size ) {
    const char * istring = stringlist_iget(stringlist , index);
    if (istring != NULL)
      if (strcmp(istring , s) == 0)
        int_vector_append(indicies, index);
    index++;
  }
  return indicies;
}


/**
  Find the index of the first index matching 's'.
  Returns -1 if 's' cannot be found.
*/
int stringlist_find_first(const stringlist_type * stringlist, const char * s) {
  bool found = false;
  int size   = stringlist_get_size( stringlist );
  int index  = 0;

  while( index < size && !found )
  {
    const char * istring = stringlist_iget(stringlist , index);
    if (istring != NULL)
      if (strcmp(istring , s) == 0)
      {
        found = true;
        break;
      }
    index++;
  }

  if(found)
    return index;
  else
    return -1;
}



bool stringlist_equal(const stringlist_type * s1 , const stringlist_type *s2) {
  int size1 = stringlist_get_size( s1 );
  int size2 = stringlist_get_size( s2 );
  if (size1 == size2) {
    bool equal = true;
    for (int i = 0; i < size1; i++) {
      if (strcmp(stringlist_iget(s1 , i) , stringlist_iget(s2 , i)) != 0) {
	equal = false;
	break;
      }
    }
    return equal;
  } else
    return false;
}


/**
   The interval is halfopen: [start_index , end_index).
*/

/* Based on buffer?? */

char * stringlist_alloc_joined_segment_string( const stringlist_type * s , int start_index , int end_index , const char * sep ) {
  char * string = NULL;
  int i;

  /* Start with allocating a string long enough to hold all the substrings. */
  {
    int sep_length   = strlen( sep );
    int total_length = 0;
    for (i=start_index; i < end_index; i++)
      total_length += (strlen(stringlist_iget( s , i)) + sep_length);

    total_length += (1 - sep_length);
    string    = util_malloc( total_length * sizeof * string , __func__);
    string[0] = '\0';
  }
  
  for (i = start_index; i < end_index; i ++) {
    strcat( string , stringlist_iget( s , i));
    if (i < (end_index - 1))
      strcat( string , sep );
  }
  
  return string;
}


char * stringlist_alloc_joined_string(const stringlist_type * s , const char * sep) {
  return stringlist_alloc_joined_segment_string( s , 0 , stringlist_get_size( s ) , sep );
}

/**
   This function will allocate a stringlist instance based on
   splitting the input string. If the input string is NULL the
   function will return a stringlist instance with zero elements.

   Observe that the splitting is based on __ANY__ character in @sep;
   NOT the full exact string @sep.

   The newly allocated stringlist will take ownership of the strings
   in the list. The actual functionality is in the util_split_string()
   function.
*/
   


stringlist_type * stringlist_alloc_from_split( const char * input_string , const char * sep ) {
  stringlist_type * slist = stringlist_alloc_new();
  if (input_string != NULL) {
    char ** items;
    int     num_items , i;
    util_split_string( input_string , sep , &num_items , &items);
    for ( i =0; i < num_items; i++)
      stringlist_append_copy( slist , items[i] );
    util_free_stringlist( items , num_items );
  }
  return slist;
}

/*****************************************************************/

void stringlist_buffer_fwrite( const stringlist_type * s , buffer_type * buffer ) {
  int i;
  int size = stringlist_get_size( s );
  buffer_fwrite_int( buffer , size );
  for (i=0; i < size; i++) 
    buffer_fwrite_string(buffer , stringlist_iget(s , i) );
}


void stringlist_fwrite(const stringlist_type * s, FILE * stream) {
  int i;
  int size = stringlist_get_size( s );
  util_fwrite_int( size , stream);
  for (i=0; i < size; i++) 
    util_fwrite_string(stringlist_iget(s , i) , stream);
}

/* 
   When a stringlist is loaded from file the current content of the
   stringlist is discarded; and the stringlist becomes the owner of
   all the data read in.
*/
void  stringlist_fread(stringlist_type * s, FILE * stream) {
  int size = util_fread_int(stream);
  int i;
  stringlist_clear(s);
  for (i=0; i < size; i++)
    stringlist_append_owned_ref( s , util_fread_alloc_string( stream ));
}


void stringlist_buffer_fread( stringlist_type * s , buffer_type * buffer ) {
  int size = buffer_fread_int( buffer );
  int i;
  stringlist_clear(s);
  for (i=0; i < size; i++)
    stringlist_append_owned_ref( s , buffer_fread_alloc_string( buffer ));
}



stringlist_type * stringlist_fread_alloc(FILE * stream) {
  stringlist_type * s = stringlist_alloc_empty( true );
  stringlist_fread(s , stream);
  return s;
}


static int strcmp__(const void * __s1, const void * __s2) {
  const char * s1 = (const char *) __s1;
  const char * s2 = (const char *) __s2;
  return strcmp( s1, s2);
}



/**
   Will sort the stringlist inplace. The prototype of the comparison
   function is

     int (cmp) (const void * , const void *);

   i.e. ths strings are implemented as (void *). If string_cmp == NULL
   the sort function will use the ordinary strcmp() function for
   comparison.
*/


void stringlist_sort(stringlist_type * s , string_cmp_ftype * string_cmp)
{
  if (string_cmp == NULL)
    vector_sort( s->strings , strcmp__ );
  else
    vector_sort( s->strings , string_cmp );
}


/*****************************************************************/

/*
  This function uses the stdlib function glob() to select file/path
  names matching a pattern. The stringlist is cleared when the
  function starts.
*/


int stringlist_select_matching(stringlist_type * names , const char * pattern) {
  int match_count = 0;
  stringlist_clear( names );
  {
    glob_t * pglob  = util_malloc( sizeof * pglob , __func__);
    int glob_flags = 0;
    int i;
    glob( pattern , glob_flags , NULL , pglob);
    match_count = pglob->gl_pathc;
    for (i=0; i < pglob->gl_pathc; i++)
      stringlist_append_copy( names , pglob->gl_pathv[i] );
    globfree( pglob );  /* Only frees the _internal_ data structures of the pglob object. */
    free( pglob );
  }
  return match_count;
}


