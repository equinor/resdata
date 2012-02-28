/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_grav_common.c' is part of ERT - Ensemble based
   Reservoir Tool.
    
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
#include <util.h>
#include <ecl_kw.h>
#include <ecl_file.h>
#include <ecl_grid_cache.h>
#include <ecl_kw_magic.h>



bool * ecl_grav_common_alloc_aquifer_cell( const ecl_grid_cache_type * grid_cache , const ecl_file_type * init_file) {
  bool * aquifer_cell = util_malloc( sizeof * aquifer_cell * ecl_grid_cache_get_size( grid_cache ) , __func__ );
  
  if (ecl_file_has_kw( init_file , AQUIFER_KW)) {
    ecl_kw_type * aquifer_kw = ecl_file_iget_named_kw( init_file , AQUIFER_KW , 0);
    const int * aquifer_data = ecl_kw_get_int_ptr( aquifer_kw );
    int active_index;
    
    for (active_index = 0; active_index < ecl_grid_cache_get_size( grid_cache ); active_index++) {
      if (aquifer_data[ active_index ] < 0)
        aquifer_cell[ active_index ] = true;
      else
        aquifer_cell[ active_index ] = false;
    }
  }
  
  return aquifer_cell;
}
