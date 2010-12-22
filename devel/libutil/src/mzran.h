#ifndef __MZRAN_H__
#define __MZRAN_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <util.h>

typedef struct mzran_struct mzran_type;

/*
  This enum enumerates different ways to initialize the mzran rng.
*/

typedef enum {
  INIT_NONE        = 0,   /* The rng is initialized with the default seed values. */
  INIT_CLOCK       = 1,   /* Four random seeds are calculated with the util_clock_seed() function. */
  INIT_DEV_RANDOM  = 2,   /* Random content is read with the function util_fread_dev_random(). */
  INIT_DEV_URANDOM = 3    /* Random content is read with the function util_fread_dev_urandom(). */
} mzran_init_mode;


UTIL_SAFE_CAST_HEADER( mzran )

void              mzran_fscanf_state( mzran_type * rng , FILE * stream );
unsigned int      mzran_sample(mzran_type * rng);
mzran_type      * mzran_alloc( mzran_init_mode init_mode );
void              mzran_init( mzran_type * rng , mzran_init_mode init_mode );
void              mzran_free( mzran_type * rgn);
void              mzran_set_state1(mzran_type * rng , int buffer_size , char * seed_buffer);
void              mzran_set_state4(mzran_type * rng , unsigned int s0 , unsigned int s1 , unsigned int s2 , unsigned int s3);
double            mzran_get_double(mzran_type * rng);
int               mzran_get_int( mzran_type * rng, int max);
void              mzran_fprintf_state( const mzran_type * rng , FILE * stream);

#ifdef __cplusplus
}
#endif
#endif
