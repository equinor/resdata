#include <string.h>
#include <util.h>
#include <hash.h>
#include <subst.h>

/**
   This file implements a small support struct for search-replace
   operations, along with wrapped calls to
   util_string_replace_inplace().
   
   Substitutions can be carried out on files and string in memory
   (char * with \0 termination); and the operations can be carried out
   inplace, or in a filtering mode where a new file/string is created.
*/


typedef enum { subst_deep_copy   = 1,     
	       subst_managed_ref = 2,
	       subst_shared_ref  = 3} subst_insert_type; /* Mode used in the subst_list_insert__() function */
  

struct subst_list_struct {
  hash_type * data;
};



/* 
   The subst_list type is implemented as a hash of subst_list_node
   instances. This node type is not exported out of this file scope at
   all.
*/
   

typedef struct {  
  bool       value_owner;     /* Wether the memory pointed to by value should bee freed.*/
  char     * value;
} subst_list_node_type;



/** Allocates an empty instance with no values. */
static subst_list_node_type * subst_list_node_alloc() {
  subst_list_node_type * node = util_malloc(sizeof * node , __func__);
  node->value       = NULL;
  node->value_owner = false;
  return node;
}


static void subst_list_node_free(subst_list_node_type * node) {
  if (node->value_owner)
    util_safe_free(node->value);
  free(node);
}


static void subst_list_node_free__(void * node) {
  subst_list_node_free( (subst_list_node_type *) node );
}

/**
   __value can be NULL. 
*/
static void subst_list_node_set(subst_list_node_type * node, const char * __value , subst_insert_type insert_mode) {
  char * value;
  /* Free the current content of the node. */
  if (node->value_owner)
    node->value = util_safe_free(node->value);

  if (insert_mode == subst_deep_copy)
    value = util_alloc_string_copy(__value);
  else
    value = (char *) __value;
  
  if (insert_mode == subst_shared_ref)
    node->value_owner = false;
  else
    node->value_owner = true;
  
  node->value = value;
}



subst_list_type * subst_list_alloc() {
  subst_list_type * subst_list = util_malloc(sizeof * subst_list , __func__);
  subst_list->data = hash_alloc();
  return subst_list;
}


static void subst_list_insert__(subst_list_type * subst_list , const char * key , const char * value , subst_insert_type insert_mode) {
  if (!hash_has_key(subst_list->data , key)) 
    hash_insert_hash_owned_ref(subst_list->data , key , subst_list_node_alloc() , subst_list_node_free__);
  {
    subst_list_node_type * node = hash_get(subst_list->data , key);
    subst_list_node_set(node , value , insert_mode);
  }
}


/**
   There are three different functions for inserting a key-value pair
   in the subst_list instance. The difference between the three is in
   which scope takes/has ownership of 'value'. The alternatives are:

    subst_list_insert_ref: In this case the calling scope has full
       ownership of value, and is consquently responsible for freeing
       it, and ensuring that it stays a valid pointer for the subst_list
       instance. Probably the most natural function to use when used
       with static storage, i.e. typically string literals.

    subst_list_insert_owned_ref: In this case the subst_list takes
       ownership of the value reference, in the sense that it will
       free it when it is done.

    subst_list_insert_copy: In this case the subst_list takes a copy
       of value and inserts it. Meaning that the substs_list instance
       takes repsonibility of freeing, _AND_ the calling scope is free
       to do whatever it wants with the value pointer.

*/
   
void subst_list_insert_ref(subst_list_type * subst_list , const char * key , const char * value) {
  subst_list_insert__(subst_list , key , value , subst_shared_ref);
}

void subst_list_insert_owned_ref(subst_list_type * subst_list , const char * key , const char * value) {
  subst_list_insert__(subst_list , key , value , subst_managed_ref);
}

void subst_list_insert_copy(subst_list_type * subst_list , const char * key , const char * value) {
  subst_list_insert__(subst_list , key , value , subst_deep_copy);
}


void subst_list_free(subst_list_type * subst_list) {
  hash_free( subst_list->data );
  free(subst_list);
}



/**
   This is the lowest level function, doing all the search/replace
   work (in this scope).
*/

static void subst_list_inplace_update_buffer__(const subst_list_type * subst_list , char ** buffer) {
  int buffer_size  = strlen( *buffer );
  const char * key = hash_iter_get_first_key( subst_list->data );

  while (key != NULL) {
    const subst_list_node_type * node = hash_get( subst_list->data , key);
    const char * value = node->value;
    if (value != NULL)
      util_string_replace_inplace( buffer , &buffer_size , key , value);
    key = hash_iter_get_next_key( subst_list->data );
  }

}
    


/**
   This function reads the content of a file, and writes a new file
   where all substitutions in subst_list have been performed. Observe
   that target_file and src_file *CAN* point to the same file, in
   which case this will amount to an inplace update. In that case a
   backup file is written, and held, during the execution of the
   function.
*/


void subst_list_filter_file(const subst_list_type * subst_list , const char * src_file , const char * target_file) {
  char * buffer;
  char * backup_file = NULL;
  if (util_same_file(src_file , target_file)) {
    char * backup_prefix = util_alloc_sprintf("%s-%s" , src_file , __func__);
    util_string_tr( backup_prefix , UTIL_PATH_SEP_CHAR , '_' );
    backup_file = util_alloc_tmp_file("/tmp" , backup_prefix , false);
    free(backup_prefix);
  }
  buffer = util_fread_alloc_file_content( src_file , NULL , NULL);
  /* Writing backup file */
  if (backup_file != NULL) {
    FILE * stream = util_fopen(backup_file , "w");
    util_fwrite( buffer , 1 , strlen(buffer) , stream , __func__);
    fclose(stream);
  }

  /* Doing the actual update */
  subst_list_inplace_update_buffer__(subst_list , &buffer);
  
  /* Writing updated file */
  {
    FILE * stream = util_fopen(target_file , "w");
    util_fwrite( buffer , 1 , strlen(buffer) , stream , __func__);
    fclose(stream);
  }

  /* OK - all went hunka dory - unlink the backup file and leave the building. */
  if (backup_file != NULL) unlink( backup_file );
  free(buffer);
}


/**
   This function does search-replace on a file inplace.
*/
void subst_list_update_file(const subst_list_type * subst_list , const char * file) {
  subst_list_filter_file( subst_list , file , file );
}


/**
   This function does search-replace on string instance inplace.
*/
void subst_list_update_string(const subst_list_type * subst_list , char ** string) {
  subst_list_inplace_update_buffer__(subst_list , string);
}



/**
   This function allocates a new string where the search-replace
   operation has been performed.
*/

char * subst_list_alloc_filtered_string(const subst_list_type * subst_list , const char * string) {
  char * buffer = util_alloc_string_copy( string );
  subst_list_inplace_update_buffer__(subst_list , &buffer);
  return buffer;
}



/**
   This function writes a filtered version of string to a file.
*/
void subst_list_filtered_fprintf(const subst_list_type * subst_list , const char * string , FILE * stream) {
  char * copy = util_alloc_string_copy(string);
  subst_list_inplace_update_buffer__(subst_list , &copy);
  fprintf(stream , "%s" , copy);  
  free(copy);
}



/**
   This allocates a new subst_list instance, the copy process is deep,
   in the sense that all srings inserted in the new subst_list
   instance have their own storage, irrespective of the ownership in
   the original subst_list instance.
*/

subst_list_type * subst_list_alloc_deep_copy(const subst_list_type * src) {
  subst_list_type * copy = subst_list_alloc();
  const char * key = hash_iter_get_first_key( src->data );
  while (key != NULL) {
    subst_list_node_type * node = hash_get( src->data , key );
    subst_list_insert__( copy , key , node->value , subst_deep_copy);
    key = hash_iter_get_next_key( src->data );
  }
  return copy;
}
