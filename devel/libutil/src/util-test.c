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




int main(int argc , char ** argv) {
  arg_pack_type * arg_pack = arg_pack_alloc();

  arg_pack_append_int(arg_pack , 1);
  arg_pack_append_int(arg_pack , 2);
  {
    FILE * stream = util_fopen("arg_text.txt" , "r");
    arg_pack_fscanf(arg_pack , stream);
    fclose(stream);
  }
  
  arg_pack_fprintf(arg_pack , stdout);

  arg_pack_free( arg_pack );

}
