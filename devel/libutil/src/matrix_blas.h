#include <stdbool.h>
#include <matrix.h>


#ifdef __cplusplus 
extern "C" {
#endif

/** 
    The external blas routines 
*/
/*****************************************************************/
void degemm_(char * , char * , int * , int * , int * , double * , double * , int * , double * , int *  , double * , double * , int *);


/*****************************************************************/


void matrix_dgemm(const matrix_type *A , const matrix_type * B , matrix_type *C , bool transA, bool transB , double alpha , double beta);




#ifdef __cplusplus
}
#endif

