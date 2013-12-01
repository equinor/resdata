/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_nnc_export.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#ifndef __ECL_NNC_EXPORT__
#define __ECL_NNC_EXPORT__

#ifdef __cplusplus
extern "C" {
#endif

#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_file.h>

typedef struct {
  size_t grid_nr1;
  size_t global_index1;
  size_t grid_nr2;
  size_t global_index2;

  double trans;
} ecl_nnc_type;


  int   ecl_nnc_export_get_size( ecl_grid_type * grid );
  void  ecl_nnc_export( const ecl_grid_type * grid , const ecl_file_type * init_file , ecl_nnc_type * nnc_data);


#ifdef __cplusplus
}
#endif
#endif
