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



int double_cmp(const void * s1,  const void *s2) {
  const double * d1 = (const double *) s1;
  const double * d2 = (const double *) s2;

  if (d1[0] < d2[0])
    return -1;
  else
    return 1;
}


int main(int argc , char ** argv) {
  vector_type * vector = vector_alloc_new();
  double * A = util_malloc(2 * sizeof * A , __func__);
  double * B = util_malloc(2 * sizeof * A , __func__);
  double * C = util_malloc(2 * sizeof * A , __func__);
  double * D = util_malloc(2 * sizeof * A , __func__);
  double * E = util_malloc(2 * sizeof * A , __func__);
  
  {
    int i;
    for (i=0; i < 2; i++) {
      A[i] = 10;
      B[i] = 5;
      C[i] = 3;
      D[i] = 671;
      E[i] = 0;
    }
  }
  
  vector_push_buffer(vector , A , 16);
  vector_push_buffer(vector , B , 16);
  vector_push_buffer(vector , C , 16);
  vector_append_buffer(vector , D , 16);
  vector_push_buffer(vector , E , 16);
  vector_sort( vector , double_cmp);
  {
    for (int i=0; i < vector_get_size(vector); i++)
      printf("vector[%d][0] = %g \n",i, ((double *)vector_iget( vector , i))[0]);
  }
  vector_free( vector );
  free(A);
  free(B);
  free(C);
  free(D);
  free(E);
}
