/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_rft.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/ecl/ecl_rft_file.h>


void test_rft( const char * rft_file ) {
  ecl_rft_file_type * rft = ecl_rft_file_alloc( rft_file );
  const ecl_rft_node_type * rft_node = ecl_rft_file_iget_node( rft , 0 );
  
  test_assert_true( ecl_rft_node_is_RFT( rft_node ));

  ecl_rft_file_free( rft );
}



void test_plt( const char * plt_file ) {
  ecl_rft_file_type * plt = ecl_rft_file_alloc( plt_file );
  const ecl_rft_node_type * plt_node = ecl_rft_file_iget_node( plt , 11 );

  test_assert_true( ecl_rft_node_is_PLT( plt_node ));
  
  ecl_rft_file_free( plt );
}



int main( int argc , char ** argv) {
  const char * rft_file = argv[1];
  const char * mode_string = argv[2];

  if (strcmp( mode_string , "RFT") == 0)
    test_rft( rft_file );
  else if (strcmp( mode_string , "PLT") == 0)
    test_plt( rft_file );
  else
    test_error_exit("Second argument:%s not recognized. Valid values are: RFT and PLT" , mode_string);
  
  exit(0);
}
