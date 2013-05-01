/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'well_segment_collection.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <ert/util/stringlist.h>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.h>


#include <ert/ecl_well/well_segment_collection.h>


int main(int argc , char ** argv) {
  test_install_SIGNALS();
  
  well_segment_collection_type * sc = well_segment_collection_alloc();
  test_assert_not_NULL( sc );
  test_assert_int_equal( well_segment_collection_get_size( sc ) , 0 );
  well_segment_collection_free( sc );
  
  exit(0);
}
