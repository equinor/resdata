/*
   Copyright (C) 2014  Statoil ASA, Norway. 
    
   The file 'fault_block_collection.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#ifndef __FAULT_BLOCK_COLLECTION_H__
#define __FAULT_BLOCK_COLLECTION_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <ert/util/type_macros.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/fault_block_layer.h>

  UTIL_IS_INSTANCE_HEADER(fault_block_collection);
  
  typedef struct fault_block_collection_struct  fault_block_collection_type;
  
  fault_block_collection_type * fault_block_collection_alloc( const ecl_grid_type * grid , const ecl_kw_type * fault_block_kw);
  void                          fault_block_collection_free( fault_block_collection_type * collection );
  int                           fault_block_collection_num_layers( const fault_block_collection_type * collection);
  fault_block_layer_type      * fault_block_collection_get_layer( const fault_block_collection_type * collection , int layer);
  
#ifdef __cplusplus
}
#endif
#endif
