/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_sum_test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/ecl/ecl_sum.h>


void test_time_range( const ecl_sum_type * ecl_sum ) {
  // Hardcoded Gurbat case values
  time_t start = util_make_date( 1,1,2000);
  time_t end   = util_make_date( 31,12,2004 );

  test_assert_time_t_equal( ecl_sum_get_start_time( ecl_sum ) , start );
  test_assert_time_t_equal( ecl_sum_get_end_time( ecl_sum )   , end );
  test_assert_time_t_equal( ecl_sum_get_data_start(ecl_sum) , start);
}






int main( int argc , char ** argv) {
  const char * casename = argv[1];

  ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case( casename , ":");

  test_time_range( ecl_sum );

  exit(0);
}
