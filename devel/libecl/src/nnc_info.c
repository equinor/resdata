
/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'nnc_info.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/ecl/nnc_info.h>
#include <stdlib.h>
#include <ert/util/util.h>


nnc_info_type * nnc_info_alloc() {
  nnc_info_type * nnc_info = util_malloc( sizeof * nnc_info );
  nnc_info->nnc_cell_numbers = int_vector_alloc(0,0); 
  return nnc_info; 
}

void nnc_info_add_nnc(nnc_info_type * nnc_info, int global_cell_number, int lgr_nr) {
  int_vector_append(nnc_info->nnc_cell_numbers, global_cell_number); 
}

void nnc_info_free( nnc_info_type * nnc_info ) {
  int_vector_free(nnc_info->nnc_cell_numbers);
  free (nnc_info); 
}
