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
#include <conf.h>


int main(int argc , char ** argv) {
  if(argc < 2 )
  {
    return 1;
  }
  stringlist_type * tokens;
  stringlist_type * src_files;

  int status = create_token_buffer(&tokens, &src_files, argv[1]);

  for(int i=0; i<stringlist_get_size(tokens); i++)
  {
    printf("Token no %i is \"%s\". Source file is \"%s\".\n", i, stringlist_iget(tokens, i), stringlist_iget(src_files, i));
  }

  stringlist_free(tokens);
  stringlist_free(src_files);

}
