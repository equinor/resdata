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
#include <matrix.h>
#include <matrix_lapack.h>


int main(int argc , char ** argv) {
  stringlist_type * a = stringlist_alloc_new();
  stringlist_type * b = stringlist_alloc_new();
  
  stringlist_append_copy(a, "foo");
  stringlist_append_copy(a, "bar");
  stringlist_append_copy(a, "svada");

  stringlist_append_copy(b, "blah");
  stringlist_append_copy(b, "kølle");

  stringlist_insert_stringlist_copy(a, b, 1);

  for(int i=0; i<stringlist_get_size(a); i++)
    printf("%s\n", stringlist_iget(a,i));


  stringlist_free(a);
  stringlist_free(b);

  for(int i = 0; i < 10; )
  {
    i++;
    printf("i: %i\n", i);
  }
}
