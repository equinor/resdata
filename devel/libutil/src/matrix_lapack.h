#include <matrix.h>

#ifdef __cplusplus 
extern "C" {
#endif


/**
   This enum is just a simple way to label the different ways the
   singular vectors in U and VT are returned to the calling scope. The
   low level lapack routine uses a character variable, indicated
   below.
*/

  
typedef enum {
  /* A */  DGESVD_ALL,           /* Returns all the singular vectors in U/VT. */
  /* S */  DGESVD_MIN_RETURN,    /* Return the first min(m,n) vectors in U/VT. */
  /* O */  DGESVD_MIN_OVERWRITE, /* Return the first min(m,n) vectors of U/VT by overwriteing in A. */
  /* N */  DGESVD_NONE}          /* Do not compute any singular vectors for U/VT */
  dgesvd_vector_enum;


  
void      matrix_dgesv(matrix_type * A , matrix_type * B);
void      matrix_dgesvd(dgesvd_vector_enum jobv, dgesvd_vector_enum jobvt , matrix_type * A , double * S , matrix_type * U , matrix_type * VT);


#ifdef __cplusplus
}
#endif
