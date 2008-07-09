#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <config.h> 
#include <stringlist.h>

int main(int argc , char ** argv) {
  stringlist_type * stringlist = stringlist_alloc_new();


  stringlist_append_copy(stringlist , "Hei");
  stringlist_append_copy(stringlist , "Hei");
  stringlist_append_copy(stringlist , "Hei");
  stringlist_free(stringlist);

}
