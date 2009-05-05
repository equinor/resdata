#include <stdbool.h>
#include <matrix.h>


#ifdef __cplusplus 
extern "C" {
#endif

/** 
    The external blas routines 
*/

/*****************************************************************/
void  dgemm_(char * , char * , int * , int * , int * , double * , double * , int * , double * , int *  , double * , double * , int *);
void  dgemv_(char * , int * , int * , double * , double * , int * , double * , int * , double * , double * , int * );
/*****************************************************************/


void matrix_dgemv(const matrix_type * A , const matrix_type * x , matrix_type * y , bool transA , double alpha , double beta);
void matrix_dgemm(const matrix_type *A , const matrix_type * B , matrix_type *C , bool transA, bool transB , double alpha , double beta);

void matrix_matmul(matrix_type * A, const matrix_type *B , const matrix_type * C);



#ifdef __cplusplus
}
#endif

