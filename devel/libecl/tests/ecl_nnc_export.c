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
#include <ert/ecl/ecl_kw_magic.h>

int count_kw_data( const ecl_file_type * file , const char * kw ) {
  int i;
  int count = 0;

  for (i=0; i < ecl_file_get_num_named_kw( file , kw ); i++) {
    ecl_kw_type * ecl_kw = ecl_file_iget_named_kw( file , kw , i );

    count += ecl_kw_get_size( ecl_kw );
  }
  return count;
}


void test_count(const char * name) {
  char * grid_file_name = ecl_util_alloc_filename(NULL , name , ECL_EGRID_FILE , false  , -1);
  ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
  ecl_file_type * grid_file = ecl_file_open( grid_file_name , 0 );
  
  int num_nnc = 0;

  num_nnc  = count_kw_data( grid_file , "NNC1" );
  num_nnc += count_kw_data( grid_file , "NNCG" );
  num_nnc += count_kw_data( grid_file , "NNA1");
  
  test_assert_int_equal( num_nnc , ecl_nnc_export_get_size( grid ));

  free(grid_file_name);
  ecl_grid_free( grid );
  ecl_file_close( grid_file );
}


void test_export(const char * name) {
  char * grid_file_name = ecl_util_alloc_filename(NULL , name , ECL_EGRID_FILE , false  , -1);
  char * init_file_name = ecl_util_alloc_filename(NULL , name , ECL_INIT_FILE , false  , -1);  
  ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
  ecl_file_type * grid_file = ecl_file_open( grid_file_name , 0 );
  ecl_file_type * init_file = ecl_file_open( init_file_name , 0);
  ecl_nnc_type * nnc_data1 = util_calloc( ecl_nnc_export_get_size( grid ) , sizeof( nnc_data1 ));
  ecl_nnc_type * nnc_data2 = util_calloc( ecl_nnc_export_get_size( grid ) , sizeof( nnc_data2 ));

  // Init nnc_data1 by manual scan
  ecl_nnc_export( grid , init_file , nnc_data2 );
  test_assert_int_equal( 0 , memcmp( nnc_data1 , nnc_data2 , ecl_nnc_export_get_size( grid ) * sizeof( nnc_data2 )));
  
  free( nnc_data2 );
  free( nnc_data1 );
  free(grid_file_name);
  ecl_grid_free( grid );
  ecl_file_close( grid_file );
  ecl_file_close( init_file );
}


void test_cmp() {
  ecl_nnc_type nnc1 = {.grid_nr1 = 1,
                       .grid_nr2 = 1,
                       .global_index1 = 1,
                       .global_index2 = 1 };

  ecl_nnc_type nnc2 = {.grid_nr1 = 1,   // nnc1 == nnc2
                       .grid_nr2 = 1,
                       .global_index1 = 1,
                       .global_index2 = 1 };

  ecl_nnc_type nnc3 = {.grid_nr1 = 4,   // nnc3 > nnc1
                       .grid_nr2 = 1,
                       .global_index1 = 1,
                       .global_index2 = 1 };
  

  ecl_nnc_type nnc4 = {.grid_nr1 = 4,   // nnc4 > nnc1
                       .grid_nr2 = 2,   // nnc4 > nnc2
                       .global_index1 = 1,
                       .global_index2 = 1 };
  
  ecl_nnc_type nnc5 = {.grid_nr1 = 4,   // nnc5 > nnc4
                       .grid_nr2 = 2,   
                       .global_index1 = 3,
                       .global_index2 = 1 };


  ecl_nnc_type nnc6 = {.grid_nr1 = 4,   // nnc6 > nnc5
                       .grid_nr2 = 2,   
                       .global_index1 = 3,
                       .global_index2 = 5 };

  

  test_assert_int_equal( 0 , ecl_nnc_cmp( &nnc1 , &nnc2 ));
  test_assert_int_equal( 1 , ecl_nnc_cmp( &nnc3 , &nnc1 ));
  test_assert_int_equal( 1 , ecl_nnc_cmp( &nnc4 , &nnc1 ));
  test_assert_int_equal( 1 , ecl_nnc_cmp( &nnc5 , &nnc4 ));
  test_assert_int_equal(-1 , ecl_nnc_cmp( &nnc4 , &nnc5 ));
  test_assert_int_equal(-1 , ecl_nnc_cmp( &nnc5 , &nnc6 ));
  
}


int main(int argc, char ** argv) {
  const char * base = argv[1];
  test_cmp( );
  test_count( base );
  //test_export( base );
  exit(0);
}
