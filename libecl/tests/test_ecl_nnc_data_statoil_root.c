/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'test_ecl_nnc_data_statoil.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/ecl/ecl_nnc_data.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_nnc_geometry.h>

#include <ert/ecl/ecl_endian_flip.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_kw.h>

#include <ert/util/util.h>
#include <ert/util/test_util.h>
#include <ert/util/test_work_area.h>

int find_index(ecl_nnc_geometry_type * nnc_geo, int grid1, int grid2, int indx1, int indx2) {
   int index = -1;
   int nnc_size = ecl_nnc_geometry_size( nnc_geo );
   for (int n = 0; n < nnc_size; n++) {
      ecl_nnc_pair_type * pair = ecl_nnc_geometry_iget( nnc_geo, n );
      if (pair->grid_nr1 == grid1 && pair->grid_nr2 == grid2)
          if (pair->global_index1 == indx1 && pair->global_index2 ==indx2) {
          index = n;
          break;
      }
   }
   return index;
}

void print_pair(ecl_nnc_type * exp_nnc_pairs, int n) {
   ecl_nnc_type nnc = exp_nnc_pairs[n];
   printf("**** grid1: %d\n", nnc.grid_nr1);
   printf("**** grid2: %d\n", nnc.grid_nr2);
   printf("**** indx1: %d\n", nnc.global_index1);
   printf("**** indx2: %d\n", nnc.global_index2);
   printf("**** trans: %f\n\n", nnc.trans); 
}

void try_out(const char * name)
{
   char * grid_file_name = ecl_util_alloc_filename(NULL , name , ECL_EGRID_FILE , false  , -1);
   char * init_file_name = ecl_util_alloc_filename(NULL , name , ECL_INIT_FILE , false  , -1);
   ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
   ecl_file_type * grid_file = ecl_file_open( grid_file_name , 0 );
   ecl_file_type * init_file = ecl_file_open( init_file_name , 0 );
   ecl_nnc_geometry_type * nnc_geo = ecl_nnc_geometry_alloc( grid );
   int nnc_size = ecl_nnc_geometry_size( nnc_geo );
   ecl_nnc_type * exp_nnc_pairs = util_malloc(nnc_size * sizeof * exp_nnc_pairs);
   ecl_nnc_export(grid, init_file, exp_nnc_pairs );

   printf("**** size : %d\n\n", nnc_size);
   print_pair(exp_nnc_pairs, 0);
   print_pair(exp_nnc_pairs, 3000);
   print_pair(exp_nnc_pairs, 10000);
   print_pair(exp_nnc_pairs, 20000);
   print_pair(exp_nnc_pairs, 30000);
   print_pair(exp_nnc_pairs, 40000);
   print_pair(exp_nnc_pairs, 47927);

}


void check_if_init_file(char * filename) {
   char * grid_file_name = ecl_util_alloc_filename(NULL , filename , ECL_EGRID_FILE , false  , -1);
   char * init_file_name = ecl_util_alloc_filename(NULL , filename , ECL_INIT_FILE , false  , -1);
   ecl_file_type * init_file = ecl_file_open( init_file_name , 0 );
   ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
   ecl_nnc_geometry_type * nnc_geo = ecl_nnc_geometry_alloc( grid );

   int nnc_size = ecl_nnc_geometry_size( nnc_geo );
   bool found = false;
   for (int n = 0; n < nnc_size; n++) {
      ecl_nnc_pair_type * pair = ecl_nnc_geometry_iget( nnc_geo, n );
      if (pair->grid_nr1 == pair->grid_nr2 && pair->grid_nr1 != 0)
         found = true;
   }
   if (found)
      printf(" ******************** FILE found: grid1 == grid2 != 0 \n");
   else
      printf(" ************************ NO SUCH FILE WAS FOUND\n");

   ecl_nnc_geometry_free(nnc_geo);
   ecl_grid_free(grid);
   ecl_file_close(init_file);
   free(grid_file_name);
   free(init_file_name);
}

void test_alloc_file(char * filename) {
   char * grid_file_name = ecl_util_alloc_filename(NULL , filename , ECL_EGRID_FILE , false  , -1);
   char * init_file_name = ecl_util_alloc_filename(NULL , filename , ECL_INIT_FILE , false  , -1);
   ecl_file_type * init_file = ecl_file_open( init_file_name , 0 );
   ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
   ecl_nnc_geometry_type * nnc_geo = ecl_nnc_geometry_alloc( grid );
   ecl_file_view_type * view_file = ecl_file_get_global_view( init_file );

   ecl_nnc_data_type * nnc_geo_data = ecl_nnc_data_alloc_tran(nnc_geo, view_file, init_file);
   
   int index;
   index = find_index( nnc_geo, 0, 0, 541, 14507);
   test_assert_double_equal(13.784438, ecl_nnc_data_iget_value( nnc_geo_data, index) );   

   index = find_index( nnc_geo, 0, 0, 48365, 118191);
   test_assert_double_equal(0.580284 , ecl_nnc_data_iget_value( nnc_geo_data, index) );

   index = find_index( nnc_geo, 0, 19, 42830, 211);
   test_assert_double_equal(0.571021 , ecl_nnc_data_iget_value( nnc_geo_data, index) );

   index = find_index( nnc_geo, 0, 79, 132406, 76);
   test_assert_double_equal(37.547710 , ecl_nnc_data_iget_value( nnc_geo_data, index) );

   ecl_nnc_geometry_free(nnc_geo);
   ecl_grid_free(grid);
   ecl_file_close(init_file);
   free(grid_file_name);
   free(init_file_name);

}


int main(int argc , char ** argv) {
   //try_out(argv[1]);
   test_alloc_file(argv[1]);
   //check_if_init_file(argv[2]);
   return 0;
}
