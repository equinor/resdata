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
#include <matrix_blas.h>
#include <conf.h>



int main(int argc , char ** argv) {
  matrix_type * S  = matrix_alloc(10 , 5);
  matrix_type * X3 = matrix_alloc(10 , 5);
  matrix_type * X5 = matrix_alloc( 5 , 5);

  matrix_random_init( S  );
  matrix_random_init( X3 );
  matrix_matlab_dump( S  , "S");
  matrix_matlab_dump( X3 , "X3");
  matrix_dgemm( X5 , S , X3 , true , false , 1.0 , 0.0);  /* X5 = T(S) * X3 */
  matrix_matlab_dump( X5 , "X5");
  
  matrix_free( S );
  matrix_free( X3 );
  matrix_free( X5 );
}
