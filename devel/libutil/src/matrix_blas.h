#include <stdbool.h>
#include <matrix.h>


#ifdef __cplusplus 
extern "C" {
#endif


void matrix_dgemm(matrix_type *C , const matrix_type *A , const matrix_type * B , bool transA, bool transB , double alpha , double beta);
void matrix_matmul(matrix_type * A, const matrix_type *B , const matrix_type * C);

void matrix_dgemv(const matrix_type * A , const double * x , double * y , bool transA , double alpha , double beta);
void matrix_mul_vector(const matrix_type * A , const double * x , double * y);


#ifdef __cplusplus
}
#endif

