/*
   Copyright (C) 2014  Statoil ASA, Norway. 
    
   The file 'fault_block_layer.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <ert/util/type_macros.h>
#include <ert/util/int_vector.h>
#include <ert/util/vector.h>

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/fault_block_layer.h>


#define FAULT_BLOCK_LAYER_ID 2297476


struct fault_block_layer_struct {
  UTIL_TYPE_ID_DECLARATION;
  const ecl_grid_type * grid;
  const ecl_kw_type   * fault_block_kw;
  int_vector_type     * block_map;
  vector_type         * blocks;
  int                   k;
};


UTIL_IS_INSTANCE_FUNCTION(fault_block_layer , FAULT_BLOCK_LAYER_ID);
static UTIL_SAFE_CAST_FUNCTION(fault_block_layer , FAULT_BLOCK_LAYER_ID);


fault_block_type * fault_block_layer_add_block( fault_block_layer_type * layer , int block_id) {
  if (int_vector_safe_iget( layer->block_map , block_id) < 0) {
    fault_block_type * block = fault_block_alloc( layer->grid , layer->k , block_id );
    int storage_index = vector_get_size( layer->blocks );
    
    int_vector_iset( layer->block_map , block_id , storage_index );
    vector_append_owned_ref( layer->blocks , block , fault_block_free__ );
    
    return block;
  } else
    return NULL;
}


static void fault_block_layer_assert_has_block( fault_block_layer_type * layer , int block_id) {
  fault_block_layer_add_block( layer , block_id );
}



static void fault_block_layer_scan( fault_block_layer_type * layer ) {
  int i,j;
  for (j=0; j < ecl_grid_get_ny( layer->grid ); j++) {
    for (i=0; i < ecl_grid_get_nx( layer->grid ); i++) {
      int g = ecl_grid_get_global_index3( layer->grid , i , j , layer->k );
      int block_id = ecl_kw_iget_int( layer->fault_block_kw , g );
      
      fault_block_layer_assert_has_block( layer , block_id );
      {
        fault_block_type * block = fault_block_layer_get_block( layer , block_id );
        fault_block_add_cell( block , i,j );
      }
    }
  }
}





fault_block_layer_type * fault_block_layer_alloc( const ecl_grid_type * grid , const ecl_kw_type * fault_block_kw , int k) {
  if ((k < 0) || (k >= ecl_grid_get_nz( grid )))
    return NULL;
  else if (ecl_kw_get_size( fault_block_kw) != ecl_grid_get_global_size(grid))
    return NULL;
  else if (ecl_kw_get_type( fault_block_kw ) != ECL_INT_TYPE)
    return NULL;
  else {
    fault_block_layer_type * layer = util_malloc( sizeof * layer );
    UTIL_TYPE_ID_INIT( layer , FAULT_BLOCK_LAYER_ID);
    layer->grid = grid;
    layer->k    = k;
    layer->fault_block_kw = fault_block_kw;
    layer->block_map = int_vector_alloc( 0 , -1);
    layer->blocks = vector_alloc_new();
    
    fault_block_layer_scan( layer );
    return layer;
  }
}


fault_block_type * fault_block_layer_iget_block( const fault_block_layer_type * layer , int storage_index) {
   return vector_iget( layer->blocks , storage_index );
}


fault_block_type * fault_block_layer_get_block( const fault_block_layer_type * layer , int block_id) {
  int storage_index = int_vector_safe_iget( layer->block_map , block_id);
  if (storage_index < 0)
    return NULL;
  else
    return vector_iget( layer->blocks , storage_index );
}



void fault_block_layer_del_block( fault_block_layer_type * layer , int block_id) {
  int storage_index = int_vector_safe_iget( layer->block_map , block_id);
  if (storage_index >= 0) {

    int_vector_iset( layer->block_map , block_id , -1 );
    vector_idel( layer->blocks , storage_index );
    {
      int index;

      for (index = 0; index < int_vector_size( layer->block_map ); index++) {
        int current_storage_index = int_vector_iget( layer->block_map , index );
        if (current_storage_index > storage_index)
          int_vector_iset( layer->block_map ,index , current_storage_index - 1);
      }
    }
  }
  
}



bool fault_block_layer_has_block( const fault_block_layer_type * layer , int block_id) {
  if (int_vector_safe_iget( layer->block_map , block_id) >= 0)
    return true;
  else
    return false;
}


int fault_block_layer_get_max_id( const fault_block_layer_type * layer ) {
   return int_vector_size( layer->block_map ) - 1;
}

int fault_block_layer_get_size( const fault_block_layer_type * layer ) {
   return vector_get_size( layer->blocks );
}


int fault_block_layer_get_k( const fault_block_layer_type * layer ) {
  return layer->k;
}



void fault_block_layer_free( fault_block_layer_type * layer ) {
  int_vector_free( layer->block_map );
  vector_free( layer->blocks );
  free(layer); 
}


void fault_block_layer_free__( void * arg ) {
  fault_block_layer_type * layer = fault_block_layer_safe_cast( arg );
  fault_block_layer_free( layer );
}
  





