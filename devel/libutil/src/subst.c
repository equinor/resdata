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
	       subst_shared_ref  = 3} subst_insert_type;
  

struct subst_list_struct {
  hash_type * data;
};



subst_list_type * subst_list_alloc() {
  subst_list_type * subst_list = util_malloc(sizeof * subst_list , __func__);
  subst_list->data = hash_alloc();
  return subst_list;
}


static void subst_list_insert__(subst_list_type * subst_list , const char * key , const char * value , subst_insert_type insert_mode) {
  switch(insert_mode) {
  case(subst_deep_copy):
    hash_insert_hash_owned_ref(subst_list->data , key , util_alloc_string_copy(value) , free);
    break;
  case(subst_managed_ref):
    hash_insert_hash_owned_ref(subst_list->data , key , value , free);
    break;
  case(subst_shared_ref):
    hash_insert_ref(subst_list->data , key , value);
    break;
  default:
    util_abort("%s: internal error : invalid value in switch statement \n",__func__);
  }
}



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
  int keys         = hash_get_size( subst_list->data );
  char ** key_list = hash_alloc_keylist(subst_list->data);

  {
    int ikey;
    for (ikey = 0; ikey < keys; ikey++)
      util_string_replace_inplace( buffer , &buffer_size , key_list[ikey] , hash_get( subst_list->data , key_list[ikey]));
  }

  util_free_stringlist(key_list , keys);
}




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


void subst_list_update_file(const subst_list_type * subst_list , const char * file) {
  subst_list_filter_file( subst_list , file , file );
}


void subst_list_update_string(const subst_list_type * subst_list , char ** string) {
  subst_list_inplace_update_buffer__(subst_list , string);
}


char * subst_list_alloc_filtered_string(const subst_list_type * subst_list , const char * string) {
  char * buffer = util_alloc_string_copy( string );
  subst_list_inplace_update_buffer__(subst_list , &buffer);
  return buffer;
}
