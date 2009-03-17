#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>
#include <arg_pack.h>
#include <vector.h>
#include <double_vector.h>



void test(char ** ptr) {
  double value;
  util_fread_from_buffer( &value , sizeof value , 1 , ptr); printf("Have read:%g \n",value);
}


int main(int argc , char ** argv) {
  stringlist_type * list = stringlist_alloc_new();

  stringlist_append_copy(list, "svada");
  stringlist_append_copy(list, "bjarne");
  stringlist_append_copy(list, "per");
  stringlist_append_copy(list, "jada");
  stringlist_append_copy(list, "mer_svada");

  stringlist_sort(list);

  int size = stringlist_get_size(list);

  for(int i= 0; i<size; i++)
    printf("elem %d: %s\n", i, stringlist_iget(list, i));

  stringlist_free(list); 
  
}
