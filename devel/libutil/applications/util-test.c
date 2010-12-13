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
#include <subst_list.h>
#include <subst_func.h>
#include <buffer.h>
#include <mzran.h>
#include <statistics.h>
#include <double_vector.h>



int main(int argc , char ** argv) {
  double_vector_type * limits = double_vector_alloc( 0,0);

  for (int i=0; i < 11; i++)
    double_vector_append( limits , i*1.0 );
  
  double_vector_fprintf( limits , stdout , "limits" , "%4.0f" );
  printf(" index(0.5):%d \n",  double_vector_lookup_bin( limits , 0.5 , -1));
  printf(" index(3.3):%d \n",  double_vector_lookup_bin( limits , 3.3 , -1));
  printf(" index(2.0):%d \n",  double_vector_lookup_bin( limits , 2.0  , -1));
  printf(" index(-0.5):%d \n", double_vector_lookup_bin( limits , -0.5 , -1));
  printf(" index(10.5):%d \n", double_vector_lookup_bin( limits , 10.5 , -1));
  
}


