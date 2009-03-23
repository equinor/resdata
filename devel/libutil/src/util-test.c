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
  matrix_type * matrix = matrix_alloc(3,3);
  matrix_set_all( matrix , 0);
  matrix_set(matrix , 0 , 0 , 1);
  matrix_set(matrix , 1 , 1 , 1);
  matrix_set(matrix , 2 , 2 , 1);
  matrix_fprintf(matrix , "%10.4f ", stdout);
  matrix_free(matrix);
}
