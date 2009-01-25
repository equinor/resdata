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
  

  {
    char * key = hash_iter_get_first_key( h );
    do {
      if (key != NULL) {
	printf("%s -> %s \n",key,hash_get(h , key));
      } else printf(" key == NULL \n");
      key = hash_iter_get_next_key(h);
    } while (key != NULL);
  }
  hash_free(h);
}
