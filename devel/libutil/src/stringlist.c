#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <stringlist.h>

#define STRINGLIST_ID 671855

/**
   This file implements a very thin wrapper around a list (vector) of
   strings, and the total number of strings. It is mostly to avoid
   sending both argc and argv.

   Most of the functionality is implemented through stateless
   functions in util.c
*/

typedef enum {ref         = 0,    /* Means that the stringlist only has reference to the item,
				     and it is the responsibility of the calling scope to free 
				     the memory of this string. */
	      list_owned  = 1 ,   /* The stringlist has taken ownership to a reference , i.e. it
				     is the stringlist's responsibility to free this memory.*/
	      copy        = 2}    /* The stringlist has taken a copy of the string, it is
				     obsviously the responsibility of the stringlist to
				     free this memory, whereas it is the responsibility of
				     the calling scope to handle the input string.*/ owner_type;



struct stringlist_struct {
  int           __id;       /* ID used to do run-time check of casts. */
  int           size;       /* The number of elements */
  char        **strings;    /* The actual strings - NB we allow NULL strings: Observe that  char ** implementation is implicitly exported. */
  owner_type * owner;
};




static void stringlist_fprintf__(const stringlist_type * stringlist, const char * sep , FILE * stream , bool debug) {
  if (debug) 
    for (int i = 0; i < stringlist->size; i++)
      fprintf(stream , "%s[%d]%s",stringlist->strings[i] , stringlist->owner[i], sep );
  else 
    for (int i = 0; i < stringlist->size; i++)
      fprintf(stream , "%s%s",stringlist->strings[i] , sep);
}


void stringlist_fprintf(const stringlist_type * stringlist, const char * sep , FILE * stream) {
  stringlist_fprintf__(stringlist , sep , stream , false);
}



/**
   this function appends num_append new items to the
   stringlist. Observe that this functions does not assign values
   (apart from trivial null initialization) to the newly appended
   memory, i.e. it is esential that the calling routine (from this
   file) has

     stringlist->xx = yy;

   statements.
*/

   
static void stringlist_grow__(stringlist_type * stringlist , int num_append) {
  int old_size = stringlist->size;
  stringlist->size += num_append;
  stringlist->strings = util_realloc(stringlist->strings , stringlist->size * sizeof * stringlist->strings , __func__);
  stringlist->owner   = util_realloc(stringlist->owner   , stringlist->size * sizeof * stringlist->owner   , __func__);
  {
    int i;
    for (i = old_size; i < stringlist->size; i++) {
      stringlist->strings[i] = NULL;
      stringlist->owner[i]   = ref;  
    }
  }
}


void stringlist_assert_index(const stringlist_type * stringlist , int index) {
  if (index >= stringlist->size || index <0 ) 
    util_abort("%s: sorry length(stringlist) = %d - index:%d invalid. \n",__func__ , stringlist->size , index); 
}

/**
   Sets element nr i in the stringlist to the input string 's'. If the
   list does not have that many elements we die (could grow as well??).

*/

static void stringlist_iset__(stringlist_type * stringlist , int index , const char * s , owner_type owner) {
  stringlist_assert_index(stringlist , index);
  if (stringlist->owner[index] != ref)
    util_safe_free(stringlist->strings[index]);
  
  stringlist->owner[index] = owner;
  if (owner == copy)
    stringlist->strings[index] = util_alloc_string_copy(s);
  else
    stringlist->strings[index] = (char *) s;
}




void stringlist_iset_copy(stringlist_type * stringlist , int index , const char *s) {
  stringlist_iset__(stringlist , index , s , copy);
}

void stringlist_iset_ref(stringlist_type * stringlist , int index , const char *s) {
  stringlist_iset__(stringlist , index , s , ref);
}

void stringlist_iset_owned_ref(stringlist_type * stringlist , int index , const char *s) {
  stringlist_iset__(stringlist , index , s , list_owned);
}



/**
   This function appends a copy of s into the stringlist.
*/
void stringlist_append_copy(stringlist_type * stringlist , const char * s) {
  stringlist_grow__(stringlist , 1);
  stringlist_iset_copy(stringlist , stringlist->size - 1 , s);
}

void stringlist_append_ref(stringlist_type * stringlist , const char * s) {
  stringlist_grow__(stringlist , 1);
  stringlist_iset_ref(stringlist , stringlist->size - 1 , s);
}

void stringlist_append_owned_ref(stringlist_type * stringlist , const char * s) {
  stringlist_grow__(stringlist , 1);
  stringlist_iset_owned_ref(stringlist , stringlist->size - 1 , s);
}




static stringlist_type * stringlist_alloc_empty() {
  stringlist_type * stringlist = util_malloc(sizeof * stringlist , __func__);
  stringlist->__id    = STRINGLIST_ID;
  stringlist->strings = NULL;
  stringlist->size    = 0;
  stringlist->owner   = NULL;
  return stringlist;
}




stringlist_type * stringlist_alloc_new() {
  return stringlist_alloc_empty();
}


static stringlist_type * stringlist_alloc__(const char ** argv , int argc , owner_type owner) {

  stringlist_type * stringlist = stringlist_alloc_empty();
  stringlist_grow__(stringlist , argc);
  {
    int iarg;
    for (iarg = 0; iarg < argc; iarg++) 
      stringlist_iset__(stringlist , iarg , argv[iarg] , owner);
  }
  return stringlist;
}


stringlist_type * stringlist_alloc_argv_copy(const char ** argv , int argc) {
  return stringlist_alloc__(argv , argc , copy);
}

stringlist_type * stringlist_alloc_argv_ref(const char ** argv , int argc) {
  return stringlist_alloc__(argv , argc , ref);
}

stringlist_type * stringlist_alloc_argv_owned_ref(const char ** argv , int argc) {
  return stringlist_alloc__(argv , argc , list_owned);
}


static stringlist_type * stringlist_alloc_copy__(const stringlist_type * stringlist, bool deep_copy) {
  owner_type owner;
  if (deep_copy)
    owner = copy;
  else
    owner = ref;
  return stringlist_alloc__((const char **) stringlist->strings , stringlist->size , owner);
}


/** 
    Allocates a new stringlist instance where all the new string are references to the
    string found in the existing stringlist instance.
*/ 
stringlist_type * stringlist_alloc_shallow_copy(const stringlist_type * stringlist) { 
  return stringlist_alloc_copy__(stringlist , false);
}


/**
    Allocates a new stringlist, where all the string are also copies.
*/
stringlist_type * stringlist_alloc_deep_copy(const stringlist_type * stringlist) { 
  return stringlist_alloc_copy__(stringlist , true);
}


void stringlist_insert_stringlist_copy(stringlist_type * stringlist , const stringlist_type * src) {
  int i;
  for (i = 0; i < src->size; i++)
    stringlist_append_copy(stringlist , stringlist_iget(src , i));
}

void stringlist_insert_stringlist_ref(stringlist_type * stringlist , const stringlist_type * src) {
  int i;
  for (i = 0; i < src->size; i++)
    stringlist_append_ref(stringlist , stringlist_iget(src , i));
}


/** 
    Frees all the memory contained by the stringlist.
*/
void stringlist_clear(stringlist_type * stringlist) {
int i;

 if (stringlist->size > 0) {
   
   for (i = 0; i < stringlist->size; i++)
     if (stringlist->owner[i] != ref)
       util_safe_free(stringlist->strings[i]);
   free(stringlist->strings);
   free(stringlist->owner);
   
   stringlist->strings = NULL;
   stringlist->size    = 0;
   stringlist->owner   = NULL;
 }
}


void stringlist_free(stringlist_type * stringlist) {
  stringlist_clear(stringlist);
  free(stringlist);
}


static stringlist_type * stringlist_safe_cast(void *_stringlist) {
  stringlist_type * stringlist = (stringlist_type *) _stringlist;
  if (stringlist->__id != STRINGLIST_ID) {
    util_abort("%s: run-time cast failed - aborting \n",__func__);
    return NULL;  /* Compiler shut up */
  } else 
    return stringlist;
}


void stringlist_free__(void * _stringlist) {
  stringlist_type * stringlist = stringlist_safe_cast(_stringlist);
  stringlist_free( stringlist );
}



const char * stringlist_iget(const stringlist_type * stringlist , int index) {
  stringlist_assert_index(stringlist ,index);
  return stringlist->strings[index];
}


char * stringlist_iget_copy(const stringlist_type * stringlist , int index) {
  return util_alloc_string_copy(stringlist_iget(stringlist , index));
}


int stringlist_get_size(const stringlist_type * stringlist) {
    return stringlist->size;
}

int stringlist_get_argc(const stringlist_type * stringlist) {
    return stringlist->size;
}


const char ** stringlist_get_argv(const stringlist_type * stringlist) {
  return (const char **) stringlist->strings;
}


const char ** stringlist_iget_argv(const stringlist_type * stringlist, int index) {
  if (index < stringlist->size)
    return (const char **) &stringlist->strings[index];
  else {
    util_abort("%s: index:%d invald \n",__func__ , index);
    return NULL; /* Compiler shut up. */
  }
}




/** 
    Scans the stringlist (linear scan) to see if it contains (at
    least) one occurence of 's';
*/

bool stringlist_contains(const stringlist_type * stringlist , const char * s) {
  int  index    = 0;
  bool contains = false;
  while ((index < stringlist->size) && (!contains)) {
    const char * istring = stringlist->strings[index];
    if (istring != NULL)
      if (strcmp(istring , s) == 0) contains = true;
    index++;
  }
  return contains;
}


bool stringlist_equal(const stringlist_type * s1 , const stringlist_type *s2) {
  if (s1->size == s2->size) {
    bool equal = true;
    for (int i = 0; i < s1->size; i++) {
      if (strcmp(s1->strings[i] , s2->strings[i]) != 0) {
	equal = false;
	break;
      }
    }
    return equal;
  } else
    return false;
}

char * stringlist_alloc_joined_string(const stringlist_type * s , const char * sep) {
  return util_alloc_joined_string((const char **) s->strings , s->size , sep);
}
