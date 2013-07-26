
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

#include <stdlib.h>

#include <ert/util/util.h>
#include <ert/util/vector.h>  
#include <ert/util/type_macros.h>

#include <ert/ecl/nnc_info.h>


#define NNC_INFO_TYPE_ID 675415078


struct nnc_info_struct {
  UTIL_TYPE_ID_DECLARATION;
  vector_type * lgr_nnc_map; 
}; 


UTIL_IS_INSTANCE_FUNCTION( nnc_info , NNC_INFO_TYPE_ID )


nnc_info_type * nnc_info_alloc() {
  nnc_info_type * nnc_info = util_malloc( sizeof * nnc_info );
  UTIL_TYPE_ID_INIT(nnc_info , NNC_INFO_TYPE_ID);
  nnc_info->lgr_nnc_map = vector_alloc_new(); 
  return nnc_info; 
}

void nnc_info_free( nnc_info_type * nnc_info ) {
  vector_free(nnc_info->lgr_nnc_map); 
  free (nnc_info); 
}

void nnc_info_add_nnc(nnc_info_type * nnc_info, int lgr_nr, int global_cell_number) {
  
  if (lgr_nr+1 > vector_get_size(nnc_info->lgr_nnc_map)) {
    vector_grow_NULL(nnc_info->lgr_nnc_map,lgr_nr ); 
    int_vector_type * nnc_for_lgr_vec = int_vector_alloc(0,0);
    int_vector_append(nnc_for_lgr_vec, global_cell_number);         
    vector_insert_owned_ref(nnc_info->lgr_nnc_map, lgr_nr, nnc_for_lgr_vec, (void*)int_vector_free );
  } else if (NULL == vector_iget(nnc_info->lgr_nnc_map, lgr_nr)) {
    int_vector_type * nnc_for_lgr_vec = int_vector_alloc(0,0);
    int_vector_append(nnc_for_lgr_vec, global_cell_number);   
    vector_insert_owned_ref(nnc_info->lgr_nnc_map, lgr_nr, nnc_for_lgr_vec, (void*)int_vector_free );
  } else {
    int_vector_append(vector_iget(nnc_info->lgr_nnc_map, lgr_nr), global_cell_number);
  }
}

const int_vector_type * get_nnc_to_lgr(const nnc_info_type * nnc_info, int lgr_nr) { 
  int_vector_type * ret = NULL;
  if (lgr_nr < vector_get_size(nnc_info->lgr_nnc_map)) {
    ret = vector_iget(nnc_info->lgr_nnc_map, lgr_nr); 
  }
  return ret;
}

