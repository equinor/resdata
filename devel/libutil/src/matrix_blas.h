#include <stdbool.h>
#include <matrix.h>


#ifdef __cplusplus 
extern "C" {
#endif

/** 
    The external blas routines 
*/


void matrix_dgemv(const matrix_type * A , const matrix_type * x , matrix_type * y , bool transA , double alpha , double beta);
void matrix_dgemm(matrix_type *C , const matrix_type *A , const matrix_type * B , bool transA, bool transB , double alpha , double beta);
void matrix_matmul(matrix_type * A, const matrix_type *B , const matrix_type * C);



#ifdef __cplusplus
}
#endif

