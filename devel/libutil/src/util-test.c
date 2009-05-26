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
  matrix_type * A1  = matrix_alloc(1000 , 100);
  matrix_type * A2  = matrix_alloc(1000 , 100);
  matrix_type * X5 = matrix_alloc(100  , 100);

  matrix_random_init( A1 );
  matrix_random_init( X5 );
  matrix_assign(A2 , A1);

  matrix_inplace_matmul( A1 , X5);
  matrix_inplace_matmul_mt( A2 , X5 , 16);
  
  if (matrix_equal(A1 , A2))
    printf("OK \n");
  else
    printf("NOT equal \n");
}
