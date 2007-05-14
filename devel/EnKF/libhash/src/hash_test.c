#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <hash.h>




int main(void) {
  char *s1 , *s2 , *s3 , *s4;

  hash_type      *hash;
  
  hash = hash_alloc(4);

  hash_insert_int(hash , "HEI" , 1);
  hash_insert_int(hash , "NAVN" , 2);

  s1 = malloc(100);
  s2 = malloc(100);
  s3 = malloc(100);
  s4 = malloc(100);

  hash_insert_hash_owned_ref(hash , "JA1" , s1 , free);
  hash_insert_hash_owned_ref(hash , "JA2" , s2 , free);
  hash_insert_hash_owned_ref(hash , "JA3" , s3 , free);
  hash_insert_hash_owned_ref(hash , "JA4" , s4 , free);
  
  hash_insert_int(hash , "iJA1" , 1);
  hash_insert_int(hash , "iJA2" , 1);
  hash_insert_int(hash , "iJA3" , 1);
  hash_insert_int(hash , "iJA4" , 1);
  hash_insert_int(hash , "iJA5" , 1);


  hash_free(hash);
  
  return 0;
}
