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
  int N = atof( argv[1] );
  int i;
  
  double_vector_type * data = double_vector_alloc(0 , 0);
  mzran_type * rng = mzran_alloc( INIT_DEV_RANDOM );

  for (i=0; i < N;  i++) 
    double_vector_iset( data , i , mzran_get_double( rng ));


  printf("Q( 0.10 ) = %7.4f \n",statistics_empirical_quantile( data , 0.10 ));
  printf("Q( 0.30 ) = %7.4f \n",statistics_empirical_quantile( data , 0.30 ));
  printf("Q( 0.50 ) = %7.4f \n",statistics_empirical_quantile( data , 0.50 ));
  printf("Q( 0.70 ) = %7.4f \n",statistics_empirical_quantile( data , 0.70 ));
  printf("Q( 0.90 ) = %7.4f \n",statistics_empirical_quantile( data , 0.90 ));
}


