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
#include <thread_pool2.h>




void * print_int( void * arg ) {
  int value = *( int * ) arg;
  printf("Value:%d \n",value);
  usleep( 10000 );
  return NULL;
}



int main(int argc , char ** argv) {
  const int num_jobs = 1000;
  int * arglist = util_malloc( num_jobs * sizeof * arglist , __func__);
  
  thread_pool_type * tp = thread_pool_alloc( 20 );
  for (int i = 0; i < num_jobs; i++) {
    arglist[i] = i + 100;
    thread_pool_add_job( tp , print_int , &arglist[i] );
  }
  
  printf("All jobs submitted ... \n");
  thread_pool_join( tp );
  thread_pool_free( tp );
  free( arglist );
  return 0;
}


