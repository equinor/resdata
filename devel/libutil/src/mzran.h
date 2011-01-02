#ifndef __MZRAN_H__
#define __MZRAN_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <util.h>

typedef struct mzran_struct mzran_type;

#define MZRAN_MAX_VALUE  4294967296
#define MZRAN_STATE_SIZE 16             /* Size of the seed buffer - in bytes. */


UTIL_SAFE_CAST_HEADER( mzran )

void              mzran_fscanf_state( void * __rng , FILE * stream );
unsigned int      mzran_forward(void * __rng);
void            * mzran_alloc( );
void              mzran_set_state(void * __rng , const char * seed_buffer);
double            mzran_get_double(mzran_type * rng);
int               mzran_get_int( mzran_type * rng, int max);
void              mzran_fprintf_state( const void * __rng , FILE * stream);
void              mzran_free( void * __rng );

#ifdef __cplusplus
}
#endif
#endif
