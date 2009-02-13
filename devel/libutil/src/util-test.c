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


int main(int argc , char ** argv) {
  int  new_line = '\n';
  char * line;

  printf("new_line:%d   EOF:%d\n",new_line,EOF);
  line = util_blocking_alloc_stdin_line(1000);   free(line);
  line = util_blocking_alloc_stdin_line(1000);   free(line);
  
}
