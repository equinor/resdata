/*
   Copyright (C) 2014  Statoil ASA, Norway. 
    
   The file 'fault_block_collection.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <ert/util/vector.h>

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/fault_block_layer.h>
#include <ert/ecl/fault_block_collection.h>


#define FAULT_BLOCK_COLLECTION_ID 9741076


struct fault_block_collection_struct {
  UTIL_TYPE_ID_DECLARATION;
  vector_type * layers;
  const ecl_grid_type * grid;
};


UTIL_IS_INSTANCE_FUNCTION(fault_block_collection , FAULT_BLOCK_COLLECTION_ID);


/*
  To facilitate on demand lazy loading we will hold on to a reference
  to the grid and fault_block_kw variables; i.e. these must stay valid
  for the lifetime of the fault_block_collection instance.
*/


fault_block_collection_type * fault_block_collection_alloc( const ecl_grid_type * grid ) {
  fault_block_collection_type * collection = util_malloc( sizeof * collection );
  UTIL_TYPE_ID_INIT( collection , FAULT_BLOCK_COLLECTION_ID);
    
  collection->grid = grid;
  collection->layers = vector_alloc_new();
    
  {
    int k;
    for (k = 0; k < ecl_grid_get_nz( collection->grid ); k++) {
      fault_block_layer_type * layer = fault_block_layer_alloc( collection->grid , k);
      vector_append_owned_ref( collection->layers , layer , fault_block_layer_free__ );
    }
  }
  return collection;
}



int fault_block_collection_num_layers( const fault_block_collection_type * collection) {
  return ecl_grid_get_nz( collection->grid );
}



void fault_block_collection_free( fault_block_collection_type * collection ) {
  vector_free( collection->layers );
  free(collection); 
}


bool fault_block_collection_scan_kw(fault_block_collection_type * collection , const ecl_kw_type * fault_block_kw) {
  if (ecl_kw_get_size( fault_block_kw) != ecl_grid_get_global_size(collection->grid))
    return false;
  else if (ecl_kw_get_type( fault_block_kw ) != ECL_INT_TYPE)
    return false;
  else {
    int k;
    for (k = 0; k < ecl_grid_get_nz( collection->grid ); k++) {
      fault_block_layer_type * layer = vector_iget( collection->layers , k );
      fault_block_layer_scan_kw( layer , fault_block_kw);
    }
    return true;
  }
}


fault_block_layer_type * fault_block_collection_get_layer( const fault_block_collection_type * collection , int k) {
  if ((k < 0) || (k >= ecl_grid_get_nz( collection->grid )))
    return NULL;
  else 
    return vector_iget( collection->layers , k );
}
  


