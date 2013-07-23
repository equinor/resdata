/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'nnc_info.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#ifndef __NNC_INFO_H__
#define __NNC_INFO_H__
#ifdef __cplusplus
extern "C" {
#endif
  
#include <ert/util/int_vector.h>  

typedef struct {
  int_vector_type * nnc_cell_numbers;
} nnc_info_type;

  nnc_info_type * nnc_info_alloc();   
  void            nnc_info_add_nnc(nnc_info_type * nnc_info, int global_cell_number, int lgr_nr); 
  void            nnc_info_free( nnc_info_type * nnc_info );
 
  
#ifdef __cplusplus
}
#endif
#endif

