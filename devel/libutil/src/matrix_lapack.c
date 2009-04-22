#include <matrix.h>
#include <matrix_lapack.h>
#include <util.h>


/** 
    The external lapack routines 
*/
/*****************************************************************/
void dgesv_(int * n, int * nrhs , double * A , int * lda , long int * ipivot , double * B , int * ldb , int * info);
void dgesvd_(char * jobu , char * jobvt , int * m , int * n , double * A, int * lda , double * S , double * U , int * ldu , double * VT , int * ldvt, double * work , int * worksize , int * info);
/*****************************************************************/



/**
   This file implements (very thin) interfaces for the matrix_type to
   the lapack functions. The file does not contain any data
   structures, only functions.
*/



static void matrix_lapack_assert_fortran_layout( const matrix_type * matrix ) {
  int rows, columns, row_stride , column_stride;
  matrix_get_dims( matrix , &rows , &columns , &row_stride , &column_stride);
  if (!( (column_stride >= rows) && (row_stride == 1)))
    util_abort("%s: lapack routines require Fortran layout of memory - aborting \n",__func__);
}


static void matrix_lapack_assert_square(const matrix_type * matrix) {
  matrix_lapack_assert_fortran_layout(matrix );
  {
    int rows, columns, row_stride , column_stride;
    matrix_get_dims( matrix , &rows , &columns , &row_stride , &column_stride);
    if (rows != columns)
      util_abort("%s: must have square matrices \n",__func__);
  }
}


/*****************************************************************/
/**
   Solves the linear equations Ax = B. The solution is stored in B on
   return.
*/


void matrix_dgesv(matrix_type * A , matrix_type * B) {
  matrix_lapack_assert_square( A );
  matrix_lapack_assert_fortran_layout( B );
  {
    int n    = matrix_get_rows( A ); 
    int lda  = matrix_get_column_stride( A );
    int ldb  = matrix_get_column_stride( B );
    int nrhs = matrix_get_columns( B );
    long int * ipivot = util_malloc( n * sizeof * ipivot , __func__ );
    int info;
    
    dgesv_(&n , &nrhs , matrix_get_data( A ) , &lda , ipivot , matrix_get_data( B ), &ldb , &info);
    if (info != 0)
      util_abort("%s: low level lapack routine: dgesv() failed with info:%d \n",__func__ , info);
    free(ipivot);
  }
}


/*****************************************************************/
/**
   Singular Value Decomposition
*/


/**
   This little function translates between an integer identifier
   (i.e. and enum instance) to one of the characters used by the low
   level lapack routine to indicate how the singular vectors should be
   returned to the calling scope.
*/

static char dgesvd_get_vector_job( dgesvd_vector_enum vector_job) {
  char job = 'X';
  switch (vector_job) {
  case(DGESVD_ALL):
    job = 'A';
    break;
  case(DGESVD_MIN_RETURN):
    job = 'S';
    break;
  case(DGESVD_MIN_OVERWRITE):
    job = 'O';
    break;
  case(DGESVD_NONE):
    job = 'N';
    break;
  default:
    util_abort("%s: internal error - unrecognized code:%d \n",vector_job);
  }
  return job;
}



void matrix_dgesvd(dgesvd_vector_enum jobu , dgesvd_vector_enum jobvt ,  matrix_type * A , double * S , matrix_type * U , matrix_type * VT) {
  char _jobu  = dgesvd_get_vector_job( jobu );
  char _jobvt = dgesvd_get_vector_job( jobvt ); 
  int m       = matrix_get_rows( A );
  int n       = matrix_get_columns( A );
  int lda     = matrix_get_column_stride( A );
  int ldu     = matrix_get_column_stride( U );
  int ldvt    = matrix_get_column_stride( VT );
  int info    = 0;
  int min_worksize = util_int_max(3* util_int_min(m , n) + util_int_max(m , n) , 5 * util_int_min(m , n));
  double * work;
  int worksize;

  /* 
     Query the routine for optimal worksize. 
  */
  work = util_malloc( 1 * sizeof * work , __func__);
  worksize = -1;
  dgesvd_(&_jobu , &_jobvt , &m , &n , matrix_get_data( A ) , &lda , S , matrix_get_data( U ) , &ldu , matrix_get_data( VT ), &ldvt , work , &worksize , &info);
  
  /* Try to allocate optimal worksize. */
  worksize = (int) work[0];
  work = realloc( work , sizeof * work * worksize );
  if (work == NULL) {
    /* Could not allocate optimal worksize - settle for the minimum. This can not fail. */
    worksize = min_worksize;
    work = util_malloc( worksize * sizeof * work , __func__);
  }

  
  dgesvd_(&_jobu , &_jobvt , &m , &n , matrix_get_data( A ) , &lda , S , matrix_get_data( U ) , &ldu , matrix_get_data( VT ), &ldvt , work , &worksize , &info);
  free( work );
}


