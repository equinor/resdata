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
  char key[9];
  int i;
  
  hash = hash_alloc(2);
  hash_insert_string_copy(hash , "Name" , "Joakim Hove");
  hash_insert_int(hash , "Alder",34);
  hash_insert_double(hash , "Vekt" , 80.01);
  for (i=0; i < 100; i++) {
    set_rand_key(key , 8);
    hash_insert_int(hash , key , rand());
  }
  
  hash_insert_ref(hash , "Key1" , &data);
  hash_insert_ref(hash , "Keyy" , &data);
  hash_insert_ref(hash , "Keyx" , &data);
  hash_insert_ref(hash , "Key2" , &data);
  hash_insert_ref(hash , "Key3" , &data);

  
  hash_insert_ref(hash , "GGKey1" , &data);
  hash_insert_ref(hash , "GGKeyy" , &data);
  hash_insert_ref(hash , "GGKeyx" , &data);
  hash_insert_ref(hash , "GGKey2" , &data);
  hash_insert_ref(hash , "GGKey3" , &data);
  
  /*{
    char **keylist = hash_alloc_keylist(hash);
    int i;
    for (i=0; i < hash_get_size(hash); i++)
      printf("%s\n",keylist[i]);
  }
  */

  printf("Jeg heter: %s \n", hash_get_string(hash , "Name"));
  printf("Jeg er %d aar gammel og veier %g kg \n",hash_get_int(hash , "Alder") , hash_get_double(hash , "Vekt"));
  ordered_keylist = hash_alloc_ordered_keylist(hash);
  random_keylist  = hash_alloc_keylist(hash);
  {
    int i;
    for (i =0; i < hash_get_size(hash); i++) 
      printf("%d:  %s  %s \n",i,ordered_keylist[i] , random_keylist[i]);
  }

  hash_free_ext_keylist(hash , ordered_keylist);
  hash_free(hash);
  return 0;
}
