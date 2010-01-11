#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <mzran.h>


struct mzran_struct {
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


unsigned int mzran_get_int(mzran_type * rng) {
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



double mzran_get_double(mzran_type * rng) {
  return 1.0 * mzran_get_int( rng ) / MZRAN_MAX;
}




void mzran_set_state4(mzran_type * rng , 
                       unsigned int s0 , unsigned int s1,
                       unsigned int s2 , unsigned int s3) {

  rng->x = s0;
  rng->y = s1;
  rng->z = s2;
  rng->n = s3;
  rng->c = 1;  
  
}



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




mzran_type * mzran_alloc( mzran_init_mode init_mode ) {
  mzran_type * rng = util_malloc( sizeof * rng , __func__);
  mzran_init( rng , init_mode );
  return rng;
}



void mzran_free( mzran_type * rng ) {
  free( rng );
}
