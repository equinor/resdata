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
#include <arg_pack.h>
#include <vector.h>
#include <double_vector.h>
#include <matrix.h>
#include <matrix_lapack.h>
#include <matrix_blas.h>
#include <parser.h>
#include <block_fs.h>
#include <thread_pool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <subst_list.h>
#include <subst_func.h>
#include <buffer.h>


int main(int argc , char ** argv) {
  subst_func_pool_type * func_pool  = subst_func_pool_alloc();
  subst_list_type      * subst_list = subst_list_alloc( func_pool );
  buffer_type * buffer = buffer_alloc(10000);
  const char * string  = "<TALL1> + <TALL2> = ADD(<TALL1>, <TALL2>)";

  subst_func_pool_add_func( func_pool , "ADD" , "Function which adds a series of number" , subst_func_add , true , 0 , 0 );
  buffer_fwrite( buffer , string , strlen( string ) , sizeof * string);

  buffer_stream_fprintf( buffer , stdout );
  printf("\n\n");
  
  subst_list_insert_ref( subst_list     , "<TALL1>" , "10.0"  , NULL);
  subst_list_insert_ref( subst_list     , "<TALL2>" , "22.0"  , NULL);
  subst_list_update_buffer( subst_list  , buffer );
  
  buffer_stream_fprintf( buffer , stdout );
  printf("\n");
  
  buffer_free( buffer );
}


