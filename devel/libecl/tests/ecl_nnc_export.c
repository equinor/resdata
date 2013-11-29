/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_nnc_export.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <ert/util/time_t_vector.h>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.h>
#include <ert/ecl/ecl_nnc_export.h>


void test_case1(const char * name) {
  char * init_file = ecl_util_alloc_filename(NULL , name , ECL_INIT_FILE , false  , -1);
  char * grid_file = ecl_util_alloc_filename(NULL , name , ECL_EGRID_FILE , false  , -1);
  ecl_grid_type * grid = ecl_grid_alloc( grid_file );
  ecl_file_type * init = ecl_file_open( init_file , 0 );
  
  //test_assert_int_equal( 2351 , ecl_nnc_export_get_size( grid , init )); 

  free(init_file);
  free(grid_file);
}




int main(int argc, char ** argv) {
  const char * base = argv[1];
  int case_nr;

  test_assert_true( util_sscanf_int(argv[2] , &case_nr) );

  switch( case_nr) {
  case 1:
    test_case1(base);
    break;
  default:
    util_abort("%s: case_nr:%d is nore recognized \n",__func__ , case_nr);
  }
  
  exit(0);
}
