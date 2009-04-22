#include <stdbool.h>
#include <matrix.h>
#include <matrix_blas.h>



/*
  C = alpha * op(A) * op(B)  +  beta * C
*/
void matrix_dgemm(const matrix_type *A , const matrix_type * B , matrix_type *C , bool transA, bool transB , double alpha , double beta) {
  int m   = matrix_get_rows( C );
  int n   = matrix_get_columns( C );
  int lda = matrix_get_column_stride( A );
  int ldb = matrix_get_column_stride( B );
  int ldc = matrix_get_column_stride( C );
  char transA_c;
  char transB_c;
  int k;

  if (transA) 
    k   = matrix_get_rows( A );
  else
    k = matrix_get_columns( A );
  
  
  if (transA)
    transA_c = 'T';
  else
    transA_c = 'N';

  if (transB)
    transB_c = 'T';
  else
    transB_c = 'N';
  
  {
    /* Need lots of checks on dimensions +++ */
    
    degemm_(&transA_c , &transB_c , &m ,&n , &k , &alpha , matrix_get_data( A ) , &lda , matrix_get_data( B ) , &ldb , &beta , matrix_get_data( C ) , &ldc);
  }
}
