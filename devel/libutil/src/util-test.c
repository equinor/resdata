#include <stdlib.h>
#include <signal.h>
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
#include <matrix_blas.h>
#include <parser.h>
#include <block_fs.h>
#include <thread_pool.h>





int main(int argc , char ** argv) {

  char * s = util_alloc_string_copy("/private/$USER = $HOME");

  {
    char * user = util_isscanf_alloc_envvar( s , 0 );
    if (user != NULL)
      util_string_replace_inplace( &s , user , getenv( &user[1] ) , NULL , NULL );
  }

  {
    char * home = util_isscanf_alloc_envvar( s , 0 );
    if (home != NULL)
      util_string_replace_inplace( &s , home , getenv( &home[1] ) , NULL , NULL );
  }
  
  printf("%s \n",s);
  
}


