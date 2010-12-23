#include <stdlib.h>
#include <math.h>
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
#include <lookup_table.h>
#include <rng.h>


int main( int argc, char ** argv)  {
  rng_type * rng   =  rng_alloc( MZRAN , INIT_DEV_RANDOM );
  matrix_type * A  =  matrix_alloc( 12 , 12 );
  matrix_type * B  =  matrix_alloc( 12 , 12 );
  matrix_random_init( A , rng );
  matrix_assign( B , A );
  matrix_pretty_print( A , "    A " , "%8.4f" );
  matrix_inv( B );
  printf("\n");
  matrix_pretty_print( B , "inv(A)" , "%8.4f" );
  matrix_inplace_matmul( B , A );
  printf("\n");
  matrix_pretty_print( B , "    I " , "%8.4f" );
  
  matrix_free( A );
  matrix_free( B );
  rng_free( rng );
}

