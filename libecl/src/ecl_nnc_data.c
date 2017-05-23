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



static ecl_kw_type * ecl_nnc_data_get_tran_kw( const ecl_file_view_type * init_file_view , const char * kw , int lgr_nr) {
  ecl_kw_type * tran_kw = NULL;
  if (lgr_nr == 0) {
    if (strcmp(kw , TRANNNC_KW) == 0)
      if(ecl_file_view_has_kw(init_file_view, kw)) {
        tran_kw = ecl_file_view_iget_named_kw(init_file_view, TRANNNC_KW, 0);
      }
  } else {
    if ((strcmp(kw , TRANNNC_KW) == 0) ||
        (strcmp(kw , TRANGL_KW) == 0)) {
      const int file_num_kw = ecl_file_view_get_size( init_file_view );
      int global_kw_index = 0;
      bool finished = false;
      bool correct_lgrheadi = false;
      int head_index = 0;
      int steps = 0;


      while(!finished){
        ecl_kw_type * ecl_kw = ecl_file_view_iget_kw( init_file_view , global_kw_index );
        const char *current_kw = ecl_kw_get_header(ecl_kw);
        if (strcmp( LGRHEADI_KW , current_kw) == 0) {
          if (ecl_kw_iget_int( ecl_kw , LGRHEADI_LGR_NR_INDEX) == lgr_nr) {
            correct_lgrheadi = true;
            head_index = global_kw_index;
          }else{
            correct_lgrheadi = false;
          }
        }
        if(correct_lgrheadi) {
          if (strcmp(kw, current_kw) == 0) {
            steps  = global_kw_index - head_index; /* This is to calculate who fare from lgrheadi we found the TRANGL/TRANNNC key word */
            if(steps == 3 || steps == 4 || steps == 6) { /* We only support a file format where TRANNNC is 3 steps and TRANGL is 4 or 6 steps from LGRHEADI */
              tran_kw = ecl_kw;
              finished = true;
              break;
            }
          }
        }
        global_kw_index++;
        if (global_kw_index == file_num_kw)
          finished = true;
      }
    }
  }
  return tran_kw;
}


static void ecl_nnc_data_set_trans(ecl_nnc_data_type * data, ecl_nnc_geometry_type * nnc_geo, ecl_file_view_type * init_file) {

   for (int nnc_index = 0; nnc_index < data->size; nnc_index++) {
      ecl_nnc_pair_type * pair = ecl_nnc_geometry_iget( nnc_geo, nnc_index );
      int grid1 = pair->grid_nr1;
      int grid2 = pair->grid_nr2;
      ecl_kw_type * global_trannnc_kw = ecl_file_view_iget_named_kw( init_file, TRANNNC_KW , 0);
      if (grid1 == grid2) {
         if (grid1 == 0) {
            data->values[nnc_index] = ecl_kw_iget_as_double(global_trannnc_kw, pair->input_index);
         }
         else
            data->values[nnc_index] = -12;
      }
      else if (grid1 == 0) {
         ecl_kw_type *  tran_kw = ecl_nnc_data_get_tran_kw( init_file, TRANGL_KW , grid2);
         data->values[nnc_index] = ecl_kw_iget_as_double(tran_kw, pair->input_index);
      }
      else 
         data->values[nnc_index] = -12; 
   }
}


ecl_nnc_data_type * ecl_nnc_data_alloc_tran(ecl_nnc_geometry_type * nnc_geo, ecl_file_view_type * init_file, ecl_file_type * DEBUG_FILE) {
   ecl_nnc_data_type * data = util_malloc( sizeof * data );

   data->size = ecl_nnc_geometry_size( nnc_geo );
   
   data->values = util_malloc( data->size * sizeof(double));

   ecl_nnc_data_set_trans(data, nnc_geo, init_file);

   return data;
}



void ecl_nnc_data_free(ecl_nnc_data_type * data) {
   free(data->values);
   free(data);
}



int ecl_nnc_data_get_size(ecl_nnc_data_type * data) {
    return data->size;
}

const double * ecl_nnc_data_get_values( const ecl_nnc_data_type * data ) {
    return data->values;
}


double ecl_nnc_data_iget_value(const ecl_nnc_data_type * data, int index) {
    if (index < data->size)
        return data->values[index];
     else
        util_abort("%s: index value:%d out range: [0,%d) \n",__func__ , index , data->size);
}




