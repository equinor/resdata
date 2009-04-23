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
  int_vector_type * v   = int_vector_alloc( 10 , 10 );
  int_vector_type * v2;
  int_vector_append( v ,  0 );
  int_vector_append( v ,  5 );
  int_vector_append( v , -1 );
  int_vector_append( v , -10);
  v2 = int_vector_alloc_copy( v );
  int_vector_fprintf(v , stdout , NULL , "%4d");

  {
    int * perm = int_vector_alloc_sort_perm( v );
    int_vector_permute( v2 , perm );
    free( perm );
  }
  int_vector_sort( v );
  int_vector_fprintf(v , stdout , NULL , "%4d");
  int_vector_fprintf(v2 , stdout , NULL , "%4d");
  int_vector_free( v  );
  int_vector_free( v2 );

  
  //const int n         = 3;
  //const int m         = 4;
  //double * S          = util_malloc( n * sizeof * S , __func__);
  //matrix_type * A     = matrix_alloc(n , m);
  //matrix_type * U     = matrix_alloc(n , n);
  //matrix_type * VT    = matrix_alloc(m , m);
  //
  //matrix_random_init( A );
  //matrix_pretty_print( A , "A" , " %10.7f ");
  //matrix_dgesvd(DGESVD_ALL , DGESVD_ALL , A , S , U , VT);
  //printf("\n\n");
  //matrix_pretty_print(U , "U"   , " %10.7f ");
  //printf("\n\n");
  //matrix_pretty_print(VT , "VT" , " %10.7f ");
  //matrix_free( A );
  //matrix_free( U );
  //matrix_free( VT );
  //free( S );
}
