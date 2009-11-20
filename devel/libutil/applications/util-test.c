#include <stdlib.h>
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
#include <subst.h>
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




int main(int argc , char ** argv) {
  int_vector_type * iv = int_vector_alloc(0,0);
  int_vector_append(iv , 10);
  int_vector_append(iv , 0);
  int_vector_append(iv , 2);
  int_vector_append(iv , 10);
  int_vector_append(iv , 0);
  int_vector_append(iv , 0);
  int_vector_append(iv , 2);
  int_vector_append(iv , 10);
  int_vector_select_unique( iv );
  int_vector_fprintf( iv , stdout , NULL , "%3d");
  int_vector_free( iv );
}


