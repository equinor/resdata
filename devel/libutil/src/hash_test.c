#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <hash.h>




int main(void) {
  hash_type * h = hash_alloc();
  hash_insert_ref(h , "Navn"    , "Joakim Hove");
  hash_insert_ref(h , "Adresse" , "Henrik Mohnsvei 6");
  hash_insert_ref(h , "legning" , "????");

  hash_get(h , "Navn");
  hash_get(h , "Adresse");
  hash_get(h , "legning");
  

  /**
    Iteration style no 1.
  */
  {
    hash_iter_type * iter = hash_iter_alloc(h);
    while ( !hash_iter_is_complete(iter) ) {
      const char * key = hash_iter_get_next_key(iter);
      printf("%s -> %s \n",key,hash_get(h , key));
    }
    hash_iter_free(iter);
  }


  /**
    Iteration style no 2.
  */
  {
    hash_iter_type * iter = hash_iter_alloc(h);
    const char     * key  = hash_iter_get_next_key(iter);
    while ( key != NULL ) {
      printf("%s -> %s \n",key,hash_get(h , key));
      key = hash_iter_get_next_key(iter);
    }
    hash_iter_free(iter);
  }

  hash_free(h);
}
