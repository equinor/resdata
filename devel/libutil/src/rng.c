#include <util.h>
#include <rng.h>
#include <stdlib.h>
#include <mzran.h>

#define RNG_TYPE_ID 66154432


/**
   Essentially a collection of function pointers to manipulate an an
   arbitrary rng state.
*/

struct rng_struct {
  UTIL_TYPE_ID_DECLARATION;
  rng_forward_ftype    * forward;
  rng_set_state_ftype  * set_state;
  rng_alloc_ftype      * alloc_state;
  rng_free_ftype       * free_state;
  rng_fscanf_ftype     * fscanf_state;
  rng_fprintf_ftype    * fprintf_state;
  /******************************************************************/
  rng_alg_type           type;
  void                 * state;
  int                    seed_size;
  unsigned long          max_value;
  double                 inv_max;
};
  


UTIL_SAFE_CAST_FUNCTION( rng , RNG_TYPE_ID)


rng_type * rng_alloc__(rng_alloc_ftype     * alloc_state,
                       rng_free_ftype      * free_state ,
                       rng_forward_ftype   * forward    ,  
                       rng_set_state_ftype * set_state  , 
                       rng_fscanf_ftype    * fscanf_state ,
                       rng_fprintf_ftype   * fprintf_state ,
                       rng_alg_type          type , 
                       int seed_size              , 
                       unsigned long max_value) {

  rng_type * rng = util_malloc( sizeof * rng , __func__ );
  UTIL_TYPE_ID_INIT( rng , RNG_TYPE_ID )
  rng->alloc_state   = alloc_state;
  rng->free_state    = free_state; 
  rng->forward       = forward;
  rng->set_state     = set_state; 
  rng->fscanf_state  = fscanf_state; 
  rng->fprintf_state = fprintf_state; 

  rng->seed_size   = seed_size;
  rng->max_value   = max_value;
  rng->inv_max     = 1.0 / max_value;
  rng->type        = type;
  rng->state       = NULL;
  
  rng->state = rng->alloc_state();

  rng_forward( rng );
  return rng;
}


/**
   Will initialize the rng with 'random content', using the method
   specified by the @init_mode variable. 

   If you have a state which you want to reproduce deterministically you
   should use one of the mzran_set_state() functions.
*/


void rng_init( rng_type * rng , rng_init_mode init_mode ) {
  char * seed_buffer = util_malloc( rng->seed_size * sizeof * seed_buffer , __func__ );
  
  switch (init_mode) {
  case(INIT_CLOCK):
    {
      for (int i=0; i < rng->seed_size; i++)
        seed_buffer[i] = ( char ) util_clock_seed();
    }
    break;
  case(INIT_DEV_RANDOM):
    util_fread_dev_random( rng->seed_size * sizeof * seed_buffer , seed_buffer );
    break;
  case(INIT_DEV_URANDOM):
    util_fread_dev_urandom( rng->seed_size * sizeof * seed_buffer , seed_buffer );
    break;
  default:
    util_abort("%s: unrecognized init_code:%d \n",__func__ , init_mode);
  }

  rng->set_state( rng->state , seed_buffer );
  free( seed_buffer );
}


void rng_rng_init( rng_type * rng , rng_type * seed_src) {
  {
    int byte_size = rng->seed_size;
    int int_size  = rng->seed_size / 4;
    unsigned int * seed_buffer;

    if (int_size * 4 < byte_size)
      int_size += 1;

    seed_buffer = util_malloc( int_size * sizeof * seed_buffer , __func__ );
    for (int i =0; i < int_size; i++) 
      seed_buffer[i] = rng_forward( seed_src );
    
    rng->set_state( rng->state , (char *) seed_buffer );
    
    free( seed_buffer );
  }
}




rng_type * rng_alloc( rng_alg_type type , rng_init_mode init_mode ) {
  rng_type * rng;
  switch (type) {
  case(MZRAN):
    rng = rng_alloc__( mzran_alloc , 
                       mzran_free , 
                       mzran_forward , 
                       mzran_set_state , 
                       mzran_fscanf_state , 
                       mzran_fprintf_state , 
                       type , 
                       MZRAN_SEED_SIZE , 
                       MZRAN_MAX_VALUE );
    break;
  default:
    util_abort("%s: rng type:%d not recognized \n",__func__ , type);
    rng = NULL;
  }
  
  if (init_mode != INIT_DEFAULT)
    rng_init( rng , init_mode );
  
  return rng;
}



void rng_free( rng_type * rng) {
  rng->free_state( rng->state );
  free( rng );
}

void rng_fprintf_state( rng_type * rng , FILE * stream ) {
  rng->fprintf_state( rng->state , stream );
}

void rng_fscanf_state( rng_type * rng , FILE * stream ) {
  rng->fscanf_state( rng->state , stream );
}

/*****************************************************************/

unsigned int rng_forward( rng_type * rng ) {
  return rng->forward( rng->state );
}

double rng_get_double( rng_type * rng ) {
  return rng->forward( rng->state ) * rng->inv_max;
}

int rng_get_int( rng_type * rng , int max_value ) {
  return rng->forward( rng->state ) % max_value;
}

rng_alg_type  rng_get_type( const rng_type * rng ) {
  return rng->type;
}
