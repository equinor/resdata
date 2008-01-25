#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


int main (int argc , char **argv) {
  hash_type * kw_hash = hash_alloc(10);

  hash_insert_hash_owned_ref(kw_hash , "KW1"   , void_arg_alloc_double(1.0) 	    , void_arg_free__);
  hash_insert_hash_owned_ref(kw_hash , "KW2"   , void_arg_alloc_double(2.0) 	    , void_arg_free__);
  hash_insert_hash_owned_ref(kw_hash , "KW3"   , void_arg_alloc_double(3.0) 	    , void_arg_free__);
  hash_insert_hash_owned_ref(kw_hash , "Navn"   , void_arg_alloc_ptr("Joakim Hove") , void_arg_free__);
  util_filter_file("mal" , "out" , '<' , '>' , kw_hash);

  hash_free(kw_hash);
  
  return 0;
}
