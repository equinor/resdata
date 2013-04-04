/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_grid_dims.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/util/test_util.h>

#include <ert/ecl/ecl_grid.h>


void test_grid( const char * grid_filename , const char * data_filename) {
  int dims[4];

  dims[3] = 0;
  test_assert_true( ecl_grid_file_dims( grid_filename , data_filename , dims ));
  test_assert_int_equal( dims[0] , 40 );
  test_assert_int_equal( dims[1] , 64 );
  test_assert_int_equal( dims[2] , 14 );
  if (data_filename)
    test_assert_int_equal( dims[3] , 34770 );
  else
    test_assert_int_equal( dims[3] , 0 );

}



int main(int argc , char ** argv) {
  const char * EGRID_file = argv[1];
  const char * GRID_file  = argv[2];
  const char * INIT_file  = argv[3];
  
  test_grid( EGRID_file , INIT_file);
  test_grid( GRID_file , INIT_file);

  test_grid( EGRID_file , NULL);
  test_grid( GRID_file , NULL);
  
  test_assert_false( ecl_grid_file_dims( "/does/not/exists/FILE.EGRID" , NULL , NULL ));
  test_assert_false( ecl_grid_file_dims( argv[0] , NULL , NULL ));
  
  exit(0);
}
