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
  int N = 10;
  matrix_type * A  = matrix_alloc(N , N);
  matrix_type * Z  = matrix_alloc(N , N);
  double * eig     = util_malloc( sizeof * eig * N , __func__);
  matrix_random_init( A );
  {
    int i,j;
    for (i = 1; i < matrix_get_rows(A); i++) 
      for (j=0; j < i; j++) 
	matrix_iset(A , i, j , matrix_iget(A , j , i));
  }

  matrix_pretty_print( A , "A" , " %6.4f ");
  matrix_matlab_dump(A , "A");
  printf("Num eigenvalues:%d \n",matrix_dsyevx_all( true , A , eig , Z));
  {
    int i;
    for (i = 0; i < N; i++)
      printf("Eigenvalue[%d] = %g \n",i,eig[i]);
  }
  matrix_pretty_print( Z , "Z" , " %6.4f ");
  matrix_matlab_dump(Z , "Z0");
}
