#ifndef __RNG_H__
#define __RNG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <util.h>


typedef enum {
  INIT_DEFAULT      = 0,   /* The rng is initialized with the default seed values. */
  INIT_CLOCK        = 1,   /* Four random seeds are calculated with the util_clock_seed() function. */
  INIT_DEV_RANDOM   = 2,   /* Random content is read with the function util_fread_dev_random(). */
  INIT_DEV_URANDOM  = 3    /* Random content is read with the function util_fread_dev_urandom(). */
} rng_init_mode;


typedef enum {
  MZRAN = 1
} rng_alg_type;
 

typedef unsigned int ( rng_forward_ftype )        ( void * );
typedef void         ( rng_set_state_ftype )      ( void * , const char * );
typedef void *       ( rng_alloc_ftype )          ( );
typedef void         ( rng_free_ftype )           ( void * );
typedef void         ( rng_fscanf_ftype )         ( void * , FILE * );
typedef void         ( rng_fprintf_ftype )        ( const void * , FILE * );

typedef struct rng_struct rng_type;

rng_type      * rng_alloc( rng_alg_type type , rng_init_mode init_mode );
void            rng_free( rng_type * rng);
void            rng_free( rng_type * rng);
unsigned int    rng_forward( rng_type * rng );
double          rng_get_double( rng_type * rng);
void            rng_rng_init( rng_type * rng , rng_type * seed_src);
void            rng_init( rng_type * rng , rng_init_mode init_mode );
rng_alg_type    rng_get_type( const rng_type * rng );
void            rng_fprintf_state( rng_type * rng , FILE * stream );
void            rng_fscanf_state( rng_type * rng , FILE * stream );

unsigned int    rng_forward( rng_type * rng );
double          rng_get_double( rng_type * rng );
int             rng_get_int( rng_type * rng , int max_value );


UTIL_SAFE_CAST_HEADER( rng );

#ifdef __cplusplus
}
#endif
#endif
