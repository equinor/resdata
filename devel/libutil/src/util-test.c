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
  matrix_type * A     = matrix_alloc(3 , 3);
  matrix_type * B     = matrix_alloc(3 , 1);

  matrix_iset(A , 0,0 , 1.0);
  matrix_iset(A , 1,1 , 2.0);
  matrix_iset(A , 2,2 , 3.0);

  matrix_set(B , 1.0);
  
  matrix_dgesv(A , B);
  matrix_pretty_print( B , "X" , " % 10.7f ");

  matrix_free(A);
  matrix_free(B);
}
