#include <stdlib.h>
#include <math.h>
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
#include <lookup_table.h>


void test( lookup_table_type * lt , double x) {
  printf("%g => %g/%g \n",x,lookup_table_interp( lt , x ) , sin(x));
}

int main(int argc , char ** argv) {
  double_vector_type * limits = double_vector_alloc( 0,0);
  double_vector_type * values = double_vector_alloc( 0,0);

  for (int i=0; i < 100; i++) {
    double x = 1.0 * i / 99;
    double_vector_append( limits , x);
    double_vector_append( values , sin(x));
  }
  
  { 
    lookup_table_type * lt = lookup_table_alloc( limits , values , false );
    
    test( lt , 0.67 );
  }

}


