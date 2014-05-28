/*
   Copyright (C) 2014  Statoil ASA, Norway. 
    
   The file 'ecl_fault_block_layer.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.h>

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/fault_block_layer.h>





void test_create_layer1( const ecl_grid_type * grid , ecl_kw_type * fault_block_kw) {
  int k = 1;
  int i,j;

  for (j=0; j < ecl_grid_get_ny( grid ); j++) {
    for (i = 0; i < ecl_grid_get_nx( grid ); i++) {
      
      int g = ecl_grid_get_global_index3( grid , i,j,k);
      ecl_kw_iset_int( fault_block_kw , g , 4 );
    }
  }

  for (j=0; j < 3; j++) {
    for (i = 0; i < 3; i++) {
      
      int g = ecl_grid_get_global_index3( grid , i,j,k);
      ecl_kw_iset_int( fault_block_kw , g , 1 );
    }
  }

  for (j=6; j < 9; j++) {
    for (i=6; i < 9; i++) {
      
      int g = ecl_grid_get_global_index3( grid , i,j,k);
      ecl_kw_iset_int( fault_block_kw , g , 7 );
    }
  }
  {
    fault_block_layer_type * layer = fault_block_layer_alloc( grid , fault_block_kw , k );
    test_assert_true( fault_block_layer_has_block( layer , 1 ));
    test_assert_true( fault_block_layer_has_block( layer , 4 ));
    test_assert_true( fault_block_layer_has_block( layer , 7 ));
    
    
    {
      fault_block_type * block = fault_block_layer_get_block( layer , 4 );
      double x,y,z;
      
      ecl_grid_get_xyz3( grid , 4,4,k , &x, &y , &z );
      test_assert_double_equal( x , fault_block_get_xc( block ));
      test_assert_double_equal( y , fault_block_get_yc( block ));
    }
    
    
    {
      fault_block_type * block = fault_block_layer_get_block( layer , 1 );
      double x,y,z;
      
      ecl_grid_get_xyz3( grid , 1,1,k , &x, &y , &z );
      test_assert_double_equal( x , fault_block_get_xc( block ));
      test_assert_double_equal( y , fault_block_get_yc( block ));
    }
    
    
    {
      fault_block_type * block = fault_block_layer_get_block( layer , 7 );
      double x,y,z;
      
      ecl_grid_get_xyz3( grid , 7,7,k , &x, &y , &z );
      test_assert_double_equal( x , fault_block_get_xc( block ));
      test_assert_double_equal( y , fault_block_get_yc( block ));
    }
    fault_block_layer_free( layer );
  }
}


void test_create_layer0( const ecl_grid_type * grid , ecl_kw_type * fault_block_kw) {
  int k = 0;
  int i,j;
  
  for (j=0; j < ecl_grid_get_ny( grid ); j++) {
    for (i = 0; i < ecl_grid_get_nx( grid ); i++) {
      
      int g = ecl_grid_get_global_index3( grid , i,j,k);
      ecl_kw_iset_int( fault_block_kw , g , 9 );
    }
  }

  {
    fault_block_layer_type * layer = fault_block_layer_alloc( grid , fault_block_kw , k );

    test_assert_false( fault_block_layer_has_block( layer , 0 ));
    test_assert_false( fault_block_layer_has_block( layer , 1 ));
    test_assert_false( fault_block_layer_has_block( layer , 2 ));
    test_assert_false( fault_block_layer_has_block( layer , 10 ));
    
    test_assert_true( fault_block_layer_has_block( layer , 9 ));
    
    test_assert_int_equal( 9 , fault_block_layer_get_max_id( layer ));
    test_assert_int_equal( 1 , fault_block_layer_get_size( layer ));
    {
      fault_block_type * block1 = fault_block_layer_get_block( layer , 9 );
      fault_block_type * block2 = fault_block_layer_iget_block( layer , 0 );
      test_assert_ptr_equal( block1 , block2 );

      {
        double x,y,z;
        ecl_grid_get_xyz3( grid , 4,4,k , &x, &y , &z );
        test_assert_double_equal( x , fault_block_get_xc( block1 ));
        test_assert_double_equal( y , fault_block_get_yc( block1 ));
      }
    }
    
    fault_block_layer_free( layer );
  }
}



void test_create( const ecl_grid_type * grid , ecl_kw_type * fault_block_kw) {
  test_create_layer0( grid , fault_block_kw);
  test_create_layer1( grid , fault_block_kw);
}




int main(int argc , char ** argv) {
  ecl_grid_type * ecl_grid = ecl_grid_alloc_rectangular( 9 , 9 , 2 , 1 , 1 , 1 , NULL );
  ecl_kw_type * fault_blk_kw = ecl_kw_alloc("FAULTBLK" , ecl_grid_get_global_size( ecl_grid ) , ECL_INT_TYPE );
  
  test_create( ecl_grid , fault_blk_kw );
  
  ecl_grid_free( ecl_grid );
  ecl_kw_free( fault_blk_kw );
  exit(0);
}


