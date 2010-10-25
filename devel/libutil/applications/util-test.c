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
#include <mzran.h>
#include <statistics.h>
#include <double_vector.h>



int main(int argc , char ** argv) {
  int_vector_type * iv = int_vector_alloc( 0 , 0);
  for (int i=0; i < 10; i++)
    int_vector_append( iv , i );
  
  int_vector_fprintf( iv , stdout , NULL , "%2d");
  int_vector_idel_block( iv , 5 , 2 );
  int_vector_fprintf( iv , stdout , NULL , "%2d");
  int_vector_idel_block( iv , 0 , 2 );
  int_vector_fprintf( iv , stdout , NULL , "%2d");
  int_vector_idel_block( iv , 4 , 20 );
  int_vector_fprintf( iv , stdout , NULL , "%2d");
  int_vector_free( iv );
}


