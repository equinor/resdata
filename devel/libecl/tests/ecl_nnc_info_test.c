/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_nnc_info_test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <ert/util/util.h>
#include <ert/util/int_vector.h>

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/nnc_info.h>
 

int main(int argc , char ** argv) {
  nnc_info_type * nnc_info = nnc_info_alloc();   
  test_assert_true(nnc_info_is_instance(nnc_info));
  test_assert_not_NULL(nnc_info); 
  
  nnc_info_add_nnc(nnc_info, 1, 110);
  nnc_info_add_nnc(nnc_info, 1, 111);
  
  const int_vector_type * nnc_cells = get_nnc_to_lgr(nnc_info, 1); 
  test_assert_int_equal(int_vector_size(nnc_cells), 2); 
  
  const int_vector_type * nnc_cells_null = get_nnc_to_lgr(nnc_info, 2); 
  test_assert_NULL(nnc_cells_null); 
  
  nnc_info_free(nnc_info);
  
  exit(0);
}
