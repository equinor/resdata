#include <util.h>
#include <hash.h>
#include <subst.h>


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


