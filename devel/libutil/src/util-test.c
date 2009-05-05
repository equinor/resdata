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
  bool transA = true;
  bool transB = false;
  double alpha = 0.671;
  double beta  = 1.56;
  
  matrix_type * L = matrix_alloc(120 , 120);

  matrix_type * A = matrix_alloc_shared(L , 0  , 0  , 40 , 4);
  matrix_type * B = matrix_alloc_shared(L , 50 , 50 , 40 , 3);
  matrix_type * C = matrix_alloc_shared(L , 90 , 95 ,  4 , 3);
  
  matrix_random_init( L );
  matrix_random_init( A );
  matrix_random_init( B );
  matrix_random_init( C );

  matrix_matlab_dump( A , "A");
  matrix_matlab_dump( B , "B");
  matrix_matlab_dump( C , "C0");

  matrix_dgemm(A , B , C , transA , transB , alpha , beta );
  matrix_matlab_dump( C , "C2");
  matrix_pretty_print( C , "C" , " %7.4f");

  {
    FILE * stream = util_fopen("params.txt" , "w");
    fprintf(stream , "%d\n%d\n%g\n%g\n", transA , transB , alpha , beta);
    fclose( stream );
  }
  
  matrix_free( L );
  matrix_free( A );
  matrix_free( B );
  matrix_free( C );
}
