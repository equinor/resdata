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


int main(int argc , char ** argv) {
  matrix_type * A     = matrix_alloc(2,2);
  matrix_type * B     = matrix_alloc(2,2);
  matrix_set( A , 0);
  matrix_set( B , 0);
  matrix_iset(A , 0 , 0 , 1);
  matrix_iset(A , 1 , 1 , 1);

  matrix_iset(B , 0 , 0 , 15);
  matrix_iset(B , 1 , 1 , 22);
  matrix_iset(B , 1 , 0 , -1);
  matrix_iset(B , 0 , 1 , -65);

  matrix_pretty_print(B , "B" , "%10.4f ");
  matrix_inplace_matmul(B , A);
  printf("\n");
  matrix_pretty_print(B , "B" , "%10.4f ");
  matrix_free( A );
  matrix_free( B );
}
