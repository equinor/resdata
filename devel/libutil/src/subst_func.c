#include <util.h>
#include <stringlist.h>
#include <subst_func.h>
#include <stdbool.h>
#include <hash.h>
#include <math.h>

#define SUBST_FUNC_TYPE_ID  646781


struct subst_func_pool_struct {
  hash_type * func_table;
};



struct subst_func_struct {
  UTIL_TYPE_ID_DECLARATION;
  subst_func_ftype  * func;
  char              * name;
  bool                vararg;
  int                 argc_min;
  int                 argc_max; 
};



char * subst_func_eval( const subst_func_type * subst_func , const stringlist_type * args ) {
  if (!subst_func->vararg) {
    /* Checking that we have th right number of arguments. */
    int argc = stringlist_get_size( args );
    if (argc < subst_func->argc_min || argc > subst_func->argc_max) {
      fprintf(stderr,"Fatal error when appying function:%s - got %d arguments: [",subst_func->name , argc);
      stringlist_fprintf(args , " " , stderr);
      fprintf(stderr,"] expected %d-%d arguments.\n", subst_func->argc_min , subst_func->argc_max);
      util_abort("%s: Fatal error - aborting \n",__func__);
    }
  }
  
  return subst_func->func( args );
}


subst_func_type * subst_func_alloc( const char * func_name , subst_func_ftype * func , bool vararg, int argc_min , int argc_max) {
  subst_func_type * subst_func = util_malloc( sizeof * subst_func , __func__);
  
  UTIL_TYPE_ID_INIT( subst_func , SUBST_FUNC_TYPE_ID );
  subst_func->func     = func;
  subst_func->name     = util_alloc_string_copy( func_name );
  subst_func->vararg   = vararg;
  subst_func->argc_min = argc_min;
  subst_func->argc_max = argc_max;
  
  return subst_func;
}


void subst_func_free( subst_func_type * subst_func ) {
  free( subst_func->name );
  free( subst_func );
}


UTIL_SAFE_CAST_FUNCTION( subst_func , SUBST_FUNC_TYPE_ID);

static void subst_func_free__( void * arg ) {
  subst_func_free( subst_func_safe_cast( arg ));
}




/*****************************************************************/


subst_func_pool_type * subst_func_pool_alloc() {
  subst_func_pool_type * pool = util_malloc( sizeof * pool , __func__);
  pool->func_table = hash_alloc();
  return pool;
}



void subst_func_pool_free( subst_func_pool_type * pool ) {
  hash_free( pool->func_table );
  free( pool );
}


void subst_func_pool_add_func( subst_func_pool_type * pool , const char * func_name , subst_func_ftype * func , bool vararg, int argc_min , int argc_max) {
  subst_func_type * subst_func = subst_func_alloc( func_name , func , vararg , argc_min , argc_max );
  hash_insert_hash_owned_ref( pool->func_table , func_name , subst_func , subst_func_free__);
}


subst_func_type * subst_func_pool_get_func( subst_func_pool_type * pool , const char * func_name ) {
  return hash_get( pool->func_table , func_name );
}

bool subst_func_pool_has_func( subst_func_pool_type * pool , const char * func_name ) {
  return hash_has_key( pool->func_table , func_name );
}


/*****************************************************************/


char * subst_func_exp( const stringlist_type * args ) {
  double arg;
  if (util_sscanf_double( stringlist_iget(args , 0 ) , &arg)) 
    return util_alloc_sprintf("%g" , exp(arg));
  else
    return NULL;
}



char * subst_func_add( const stringlist_type * args ) {
  double sum = 0;
  bool OK = true;
  int index;
  for (index = 0; index < stringlist_get_size( args ); index++) {
    double term;
    if (util_sscanf_double( stringlist_iget(args , index ) , &term))
      sum += term;
    else
      OK = false;
  }

  if (OK)
    return util_alloc_sprintf("%g" , sum);
  else
    return NULL;
}
