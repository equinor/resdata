#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <mzran.h>

/*****************************************************************/
/*
  This file implements the mz random number generator. Historically
  the rng has been translated from Fortran code found on the internet,
  used in the context of SSE QMC simulations by Anders Sandvik.

  The state of the random number generator is based on 4 unsigned
  integers.
*/



#define MZRAN_TYPE_ID  77156432

struct mzran_struct {
  UTIL_TYPE_ID_DECLARATION;
  unsigned int x;
  unsigned int y;
  unsigned int z;
  unsigned int c;
  unsigned int n;
};


#define MZRAN_MAX 4294967296;
#define DEFAULT_S0  99
#define DEFAULT_S1 199
#define DEFAULT_S2  13
#define DEFAULT_S3  77


/*****************************************************************/


/**
   This function will return and unsigned int. This is the fundamental
   low level function which drives the random number generator state
   forward. The returned value will be in the interval [0,MZRAN_MAX).
*/

unsigned int mzran_sample(mzran_type * rng) {
  unsigned int s;
  
  if (rng->y > (rng->x + rng->c)) { 
    s = rng->y - rng->x - rng->c;    
    rng->c=0; 
  } else  { 
    s = rng->y - rng->x - rng->c - 18; 
    rng->c=1; 
  }

  rng->x = rng->y; 
  rng->y = rng->z; 
  rng->z = s; 
  rng->n = 69069*rng->n + 1013904243;
  return rng->z + rng->n;
}





/**
   Returns a double in the interval [0,1).
*/
double mzran_get_double(mzran_type * rng ) {
  return 1.0 * mzran_sample( rng ) / MZRAN_MAX;
}



int mzran_get_int( mzran_type * rng, int max) {
  return mzran_sample( rng ) % max;
}




/**
  This function will set the state of the rng, based on four input
  seeds.
*/ 
void mzran_set_state4(mzran_type * rng , 
                       unsigned int s0 , unsigned int s1,
                       unsigned int s2 , unsigned int s3) {

  rng->x = s0;
  rng->y = s1;
  rng->z = s2;
  rng->n = s3;
  rng->c = 1;  
  
}

/**
   Will load four unsigned integers from the open FILE * and call
   mzran_set_state4(). The integers will be loaded with fscanf(),
   i.e. this is formatted file. Will crash and burn if the reading fails. 
*/
   
void mzran_fscanf_state( mzran_type * rng , FILE * stream ) {
  unsigned int s0,s1,s2,s3;
  if (fscanf( stream , "%u %u %u %u" , &s0 , &s1 , &s2 , &s3) == 4)
    mzran_set_state4( rng , s0 , s1 , s2 , s3);
  else 
    util_abort("%s: failed to load for integers ---- \n",__func__);
}


void mzran_fprintf_state( const mzran_type * rng , FILE * stream) {
  fprintf(stream,"%u %u %u %u\n",rng->x , rng->y, rng->z, rng->n);
}


/**
   This function will set the state of the rng, based on a buffer of
   length buffer size. To set the state of the rng completely 16 bytes
   are needed, if the buffer is smaller than 16 bytes, the content is
   recycled.
*/

void mzran_set_state1(mzran_type * rng , int buffer_size , char * seed_buffer) {
  const int seed_size = 4 * sizeof( unsigned int );
  unsigned int seed[4];
  int seed_offset = 0;
  do {
    int copy_size = util_int_max( seed_size - seed_offset , buffer_size );
    memcpy( &seed[seed_offset] , &seed_buffer[ seed_offset ] , copy_size );
    seed_offset += copy_size;
  } while (seed_offset < seed_size);
    
  mzran_set_state4( rng , seed[0] , seed[1] , seed[2] , seed[3]);
}


/**
   Will initialize the rng with 'random content', using the method
   specified by the @init_mode variable. 

   If you have a state which you want to reproduce deterministically you
   should use one of the mzran_set_state() functions.
*/

void mzran_init( mzran_type * rng , mzran_init_mode init_mode ) {
  unsigned int seed[4];
  switch (init_mode) {
  case(INIT_NONE):
    seed[0] = DEFAULT_S0;
    seed[1] = DEFAULT_S1;
    seed[2] = DEFAULT_S2;
    seed[3] = DEFAULT_S3;
    break;
  case(INIT_CLOCK):
    seed[0] =           util_clock_seed();
    seed[1] = seed[0] * util_clock_seed();
    seed[2] = seed[1] * util_clock_seed();
    seed[3] = seed[2] * util_clock_seed();
    break;
  case(INIT_DEV_RANDOM):
    util_fread_dev_random( 4 * sizeof * seed , (char *) seed );
    break;
  case(INIT_DEV_URANDOM):
    util_fread_dev_urandom( 4 * sizeof * seed , (char *) seed );
    break;
  default:
    util_abort("%s: unrecognized init_code:%d \n",__func__ , init_mode);
  }
  mzran_set_state4( rng , seed[0] , seed[1] , seed[2] , seed[3]);
}


UTIL_SAFE_CAST_FUNCTION( mzran , MZRAN_TYPE_ID)


/**
   Creates (and initializes) a new rng instance. To recover a known
   state you must subsequently call one of the mzran_set_state()
   functions.
*/
mzran_type * mzran_alloc( mzran_init_mode init_mode ) {
  mzran_type * rng = util_malloc( sizeof * rng , __func__);
  UTIL_TYPE_ID_INIT( rng , MZRAN_TYPE_ID );
  mzran_init( rng , init_mode );
  return rng;
}





void mzran_free( mzran_type * rng ) {
  free( rng );
}
