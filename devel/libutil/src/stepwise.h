#ifndef __STEPWISE_H__
#define __STEPWISE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <matrix.h>
#include <bool_vector.h>

  typedef struct stepwise_struct stepwise_type;

  stepwise_type * stepwise_alloc3( matrix_type * Y, bool internal_copy );  
  stepwise_type * stepwise_alloc2( matrix_type * X , matrix_type * Y , bool internal_copy );
  stepwise_type * stepwise_alloc1(int nsample, int nvar, rng_type * rng);

  void            stepwise_free( stepwise_type * stepwise);
  void            stepwise_estimate( stepwise_type * stepwise , double deltaR2_limit , int CV_blocks);
  double          stepwise_eval( const stepwise_type * stepwise , const matrix_type * x );
  void            stepwise_set_Y0( stepwise_type * stepwise , const matrix_type * Y);
  void            stepwise_set_X0( stepwise_type * stepwise , const matrix_type * X);


  
  
#ifdef __cplusplus
}
#endif

#endif
