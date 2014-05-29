/*
   Copyright (C) 2014  Statoil ASA, Norway. 
    
   The file 'ecl_fault_block.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <ert/ecl/fault_block.h>




void test_create(const ecl_grid_type * grid) {
  fault_block_type * block = fault_block_alloc( grid , 0 , 65 );

  test_assert_true( fault_block_is_instance( block ));
  test_assert_int_equal( 0 , fault_block_get_size( block ));
  test_assert_int_equal( 65 , fault_block_get_id( block ));

  fault_block_add_cell( block , 0 , 0);
  test_assert_int_equal( 1 , fault_block_get_size( block ));
  {
    double x,y,z;
    
    ecl_grid_get_xyz1( grid , 0 , &x, &y , &z );
    test_assert_double_equal( x , fault_block_get_xc( block ));
    test_assert_double_equal( y , fault_block_get_yc( block ));

    
    fault_block_add_cell( block , 1 , 0 );
    fault_block_add_cell( block , 0 , 1 );
    fault_block_add_cell( block , 1 , 1  );
    test_assert_double_equal( 1.0 , fault_block_get_xc( block ));
    test_assert_double_equal( 1.0 , fault_block_get_yc( block ));
  }
  
  
  fault_block_free( block );
}


int main(int argc , char ** argv) {
  ecl_grid_type * grid = ecl_grid_alloc_rectangular( 9 , 9 , 2 , 1 , 1 , 1 , NULL );
  {
    test_create(grid);
  }
  ecl_grid_free( grid );
  exit(0);
}
