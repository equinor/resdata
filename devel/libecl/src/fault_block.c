/*
   Copyright (C) 2014  Statoil ASA, Norway. 
    
   The file 'fault_block.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/fault_block.h>


#define FAULT_BLOCK_ID 3297376


struct fault_block_struct {
  UTIL_TYPE_ID_DECLARATION;
  const ecl_grid_type * grid;
  int_vector_type * cell_list;  
  int               block_id;
  int               k;
  double            xc,yc;
  bool              valid_center;
};


UTIL_IS_INSTANCE_FUNCTION( fault_block , FAULT_BLOCK_ID )
static UTIL_SAFE_CAST_FUNCTION( fault_block , FAULT_BLOCK_ID )


fault_block_type * fault_block_alloc( const ecl_grid_type * grid ,  int k , int block_id ) {
  fault_block_type * block = util_malloc( sizeof * block );
  UTIL_TYPE_ID_INIT( block , FAULT_BLOCK_ID );
  block->grid = grid;
  block->cell_list = int_vector_alloc(0,0);
  block->valid_center = false;
  block->block_id = block_id;
  block->k = k;
  return block;
}


int fault_block_get_size( const fault_block_type * block ) {
  return int_vector_size( block->cell_list );            
}


int fault_block_get_id( const fault_block_type * block ) {
  return block->block_id;
}


void fault_block_free( fault_block_type * block ) {
  int_vector_free( block->cell_list );
  free( block );
}


void fault_block_free__( void * arg) {
  fault_block_type * block = fault_block_safe_cast( arg );
  fault_block_free( block );
}


void fault_block_add_cell( fault_block_type * fault_block , int i , int j) {
  int global_index = ecl_grid_get_global_index3( fault_block->grid , i , j , fault_block->k );
  int_vector_append( fault_block->cell_list , global_index );
  fault_block->valid_center = false;
}


static void fault_block_assert_center( fault_block_type * fault_block ) {
  if (!fault_block->valid_center) {
    int index;
    double xc = 0;
    double yc = 0;
    
    for (index = 0; index < int_vector_size( fault_block->cell_list ); index++) {
      int global_index = int_vector_iget( fault_block->cell_list , index );
      double x , y , z;

      ecl_grid_get_xyz1( fault_block->grid , global_index , &x , &y , &z);
      xc += x;
      yc += y;
    }
    
    fault_block->xc = xc / int_vector_size( fault_block->cell_list );
    fault_block->yc = yc / int_vector_size( fault_block->cell_list );
  }
  fault_block->valid_center = true;
}


double fault_block_get_xc( fault_block_type * fault_block ) {
  fault_block_assert_center( fault_block );
  return fault_block->xc;
}


double fault_block_get_yc( fault_block_type * fault_block ) {
  fault_block_assert_center( fault_block );
  return fault_block->yc;
}
