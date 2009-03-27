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
  matrix_type * A     = matrix_alloc(5,5);
  matrix_set(A , 55);
  matrix_pretty_print(A , "A" , " %10.7f ");
  
  printf("\n\n");
  
  matrix_resize(A , 7 , 3);
  matrix_pretty_print(A , "A" ," %10.7f ");
  matrix_free(A);
}
