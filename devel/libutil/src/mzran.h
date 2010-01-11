#ifndef __MZRAN_H__
#define __MZRAN_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>

typedef struct mzran_struct mzran_type;

typedef enum {
  INIT_NONE        = 0,
  INIT_CLOCK       = 1, 
  INIT_DEV_RANDOM  = 2,
  INIT_DEV_URANDOM = 3
} mzran_init_mode;



mzran_type      * mzran_alloc( mzran_init_mode init_mode );
void              mzran_init( mzran_type * rng , mzran_init_mode init_mode );
void              mzran_free( mzran_type * rgn);
void              mzran_set_state1(mzran_type * rng , int buffer_size , char * seed_buffer);
void              mzran_set_state4(mzran_type * rng , unsigned int s0 , unsigned int s1 , unsigned int s2 , unsigned int s3);
double            mzran_get_double(mzran_type * rng);
unsigned int      mzran_get_int(mzran_type * rng);


#ifdef __cplusplus
}
#endif
#endif
