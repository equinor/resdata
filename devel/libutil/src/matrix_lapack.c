#include <matrix.h>
#include <matrix_lapack.h>
#include <util.h>


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
