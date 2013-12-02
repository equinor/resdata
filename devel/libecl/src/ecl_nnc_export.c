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

#include <ert/util/int_vector.h>

#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_nnc_export.h>


int ecl_nnc_export_get_size( ecl_grid_type * grid ) {
  return ecl_grid_get_num_nnc( grid );
}


void  ecl_nnc_export( const ecl_grid_type * grid , const ecl_file_type * init_file , ecl_nnc_type * nnc_data) {
  
}



int ecl_nnc_cmp( const ecl_nnc_type * nnc1 , const ecl_nnc_type * nnc2) {

  if (nnc1->grid_nr1 != nnc2->grid_nr1) {
    if (nnc1->grid_nr1 < nnc2->grid_nr1) 
      return -1;
    else
      return 1;
  }

  if (nnc1->grid_nr2 != nnc2->grid_nr2) {
    if (nnc1->grid_nr2 < nnc2->grid_nr2) 
      return -1;
    else
      return 1;
  }

  if (nnc1->global_index1 != nnc2->global_index1) {
    if (nnc1->global_index1 < nnc2->global_index1) 
      return -1;
    else
      return 1;
  }


  if (nnc1->global_index2 != nnc2->global_index2) {
    if (nnc1->global_index2 < nnc2->global_index2) 
      return -1;
    else
      return 1;
  }
  
  return 0;
}
