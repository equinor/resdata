#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <hash.h>


void set_rand_key(char *key, int N) {
  int i;
  for (i=0; i <N; i++)
    key[i] = 65 + rand() % 26;
  key[N] = '\0';
}


int main(void) {
  double data;
  hash_type      *hash;
  char **ordered_keylist;
  char **random_keylist;
  char **sorted_keylist;
  char key[9];
  int i;
  
  hash = hash_alloc(10);
  hash_insert_int(hash , "B" , 1);
  hash_insert_int(hash , "C" , 1);
  hash_insert_int(hash , "D" , 1);
  hash_insert_int(hash , "E" , 1);
  hash_insert_int(hash , "A" , 1);
  hash_insert_int(hash , "I" , 1);
  hash_insert_int(hash , "G" , 1);
  hash_insert_int(hash , "K" , 1);
  hash_insert_int(hash , "J" , 1);
  hash_insert_int(hash , "F" , 1);
  hash_insert_int(hash , "L" , 1);
  hash_insert_int(hash , "H" , 1);

  ordered_keylist = hash_alloc_ordered_keylist(hash);
  sorted_keylist  = hash_alloc_sorted_keylist(hash);
  random_keylist  = hash_alloc_keylist(hash);

  for (i=0; i < hash_get_size(hash); i++)
    printf("%s  %s  %s  \n",sorted_keylist[i] , ordered_keylist[i] , random_keylist[i]);
  


  hash_free_ext_keylist(hash , ordered_keylist);
  hash_free(hash);
  return 0;
}
