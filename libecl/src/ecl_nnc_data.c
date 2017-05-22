/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'ecl_file_view.c' is part of ERT - Ensemble based Reservoir Tool.

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

#define ECL_NNC_DATA_TYPE_ID 83756236

#include <ert/ecl/ecl_nnc_data.h>
#include <ert/ecl/ecl_nnc_geometry.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_file_view.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw_magic.h>


struct ecl_nnc_data_struct {
   UTIL_TYPE_ID_DECLARATION;
   int size;
   double * values;
};



static ecl_kw_type * ecl_nnc_data_get_trangl_kw( const ecl_file_view_type * view_file, int lgr_nr ) {
  ecl_kw_type * tran_kw = NULL;
  return tran_kw;
}


static void ecl_nnc_data_set_trans(ecl_nnc_data_type * data, ecl_nnc_geometry_type * nnc_geo, ecl_file_view_type * view_file) {
 
   data->size = ecl_nnc_geometry_size( nnc_geo );
   
   data->values = util_malloc( data->size * sizeof(double));

   ecl_kw_type * trannnc_kw;

   if (ecl_file_view_has_kw( view_file, TRANNNC_KW ) ) {    
      trannnc_kw =  ecl_file_view_iget_named_kw( view_file, TRANNNC_KW , 0);
   }

   for (int nnc_index = 0; nnc_index < data->size; nnc_index++) {
      ecl_nnc_pair_type * pair = ecl_nnc_geometry_iget( nnc_geo, nnc_index );
      int grid1 = pair->grid_nr1;
      int grid2 = pair->grid_nr2;
      if (grid1 == grid2) {
            data->values[nnc_index] = ecl_kw_iget_as_double(trannnc_kw, pair->input_index);
      }
      else if (grid1 == 0) {
         data->values[nnc_index] = -12;
      }
      else 
         data->values[nnc_index] = -12; 
      
   }
   

}


ecl_nnc_data_type * ecl_nnc_data_alloc_tran(ecl_nnc_geometry_type * nnc_geo, ecl_file_view_type * view_file) {
   ecl_nnc_data_type * data = util_malloc( sizeof * data );

   ecl_nnc_data_set_trans(data, nnc_geo, view_file);

   return data;
}



void ecl_nnc_data_free(ecl_nnc_data_type * data) {
   free(data->values);
   free(data);
}



int ecl_nnc_data_get_size(ecl_nnc_data_type * data) {
    return data->size;
}

double * ecl_nnc_data_get_values( const ecl_nnc_data_type * data ) {
    return data->values;
}


double ecl_nnc_data_iget_value(const ecl_nnc_data_type * data, int index) {
    return data->values[index];
}




