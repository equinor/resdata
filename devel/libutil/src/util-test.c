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


int main(int argc , char ** argv) {
  const int n         = 3;
  const int m         = 4;
  double * S          = util_malloc( n * sizeof * S , __func__);
  matrix_type * A     = matrix_alloc(n , m);
  matrix_type * U     = matrix_alloc(n , n);
  matrix_type * VT    = matrix_alloc(m , m);

  matrix_random_init( A );
  matrix_pretty_print( A , "A" , " %10.7f ");
  matrix_dgesvd(DGESVD_ALL , DGESVD_ALL , A , S , U , VT);
  printf("\n\n");
  matrix_pretty_print(U , "U"   , " %10.7f ");
  printf("\n\n");
  matrix_pretty_print(VT , "VT" , " %10.7f ");
  matrix_free( A );
  matrix_free( U );
  matrix_free( VT );
  free( S );
}
