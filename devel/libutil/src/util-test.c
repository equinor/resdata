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
#include <stringlist.h>
#include <menu.h>





int main(int argc , char ** argv) {
  if(argc < 4)
  {
    printf("Usage: util.x string expr subs\n");
    return 0;
  }
  printf("original : %s\n", argv[1]);
  printf("replacing: %s --> %s\n", argv[2], argv[3]);

  char * rep = util_string_replace_alloc(argv[1], argv[2], argv[3]);
  printf("replaced : %s\n", rep);
  free(rep);
  return 0;
}
