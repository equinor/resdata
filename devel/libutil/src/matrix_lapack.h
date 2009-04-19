#include <matrix.h>

#ifdef __cplusplus 
extern "C" {
#endif

/** 
    The external lapack routines 
*/
/*****************************************************************/
void dgesv_(int * n, int * nrhs , double * A , int * lda , long int * ipivot , double * B , int * ldb , int * info);


/*****************************************************************/



void matrix_dgesv(matrix_type * A , matrix_type * B);



#ifdef __cplusplus
}
#endif
