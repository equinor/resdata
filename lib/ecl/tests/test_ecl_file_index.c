/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'test_ecl_file_index.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdio.h>

#include <ert/util/test_util.h>
#include <ert/util/util.h>
#include <ert/util/test_work_area.h>

#include <ert/ecl/ecl_endian_flip.h>
#include <ert/ecl/ecl_file.h>

void test_load_nonexisting_file() {
   ecl_file_type * ecl_file = ecl_file_fast_open("base_file", "a_file_that_does_not_exist_2384623", 0);
   test_assert_NULL( ecl_file );
}


void test_create_and_load_index_file() {
   //X1: Start w/ just one kw
   //X2: Expand to several kw 

   //***1: create/get an ecl type file
   //***2: use ecl_file_open to read from the file -> ecl_file_start
   //3: store some values from the file, V1 ...  Vn
   //4: use NEW function ecl_file_write_index to create an ecl_index file
   //5: use NEW function ecl_file_fast_open to create a new ecl_file type -> ecl_file_end
   //6: Use ecl_file_end to read from file and compare values to V1 ... Vn
   //7: Remove from work area "initial_data_file", close/free variable ecl_file_start, ecl_file_end
   //8: Create new initial_data_file
   //9: Read index_file again w/ ecl_file_fast_open, assert return of NULL

   

   
   test_work_area_type * work_area = test_work_area_alloc("ecl_file_index_testing");
   {
      const char * file_name = "initial_data_file";
      const char * index_file_name = "index_file";

      //creating the data file
      size_t data_size = 10;
      ecl_kw_type * kw = ecl_kw_alloc("TEST1_KW", data_size, ECL_INT);
      for(int i = 0; i < data_size; ++i)
         ecl_kw_iset_int(kw, i, 537 + i);
      fortio_type * fortio = fortio_open_writer(file_name, false, ECL_ENDIAN_FLIP);
      ecl_kw_fwrite(kw, fortio); 
      ecl_kw_free(kw);
      
      data_size = 5;
      kw = ecl_kw_alloc("TEST2_KW", data_size, ECL_FLOAT);
      for(int i = 0; i < data_size; ++i)
         ecl_kw_iset_float(kw, i, 0.15 * i);
      ecl_kw_fwrite(kw, fortio);
      ecl_kw_free(kw);
      fortio_fclose(fortio); 
      //finished creating data file

      //creating ecl_file
      ecl_file_type * ecl_file = ecl_file_open( file_name , 0 );
      test_assert_true( ecl_file_has_kw( ecl_file , "TEST1_KW" )  );
      ecl_file_write_index( ecl_file ,  file_name , index_file_name);
      int ecl_file_size = ecl_file_get_size( ecl_file );
      ecl_file_close( ecl_file ); 
      //finished using ecl_file

      
      ecl_file_type * ecl_file_index = ecl_file_fast_open( file_name, index_file_name , 0);
      test_assert_true( ecl_file_is_instance(ecl_file_index)  );

      //Add timestamp check

      test_assert_int_equal(ecl_file_size, ecl_file_get_size(ecl_file_index) );    
  
      test_assert_true( ecl_file_has_kw( ecl_file_index, "TEST1_KW" )  );
      printf("***************************************************\n");
      test_assert_true( ecl_file_has_kw( ecl_file_index, "TEST2_KW" )  );
      kw = ecl_file_iget_kw( ecl_file_index , 0 );  
      test_assert_double_equal( 537.0, ecl_kw_iget_as_double(kw, 0)  );
      test_assert_double_equal( 546.0, ecl_kw_iget_as_double(kw, 9)  );
      kw = ecl_file_iget_kw( ecl_file_index , 1 );
      test_assert_double_equal( 0.15, ecl_kw_iget_as_double(kw, 1)  );
      test_assert_double_equal( 0.60, ecl_kw_iget_as_double(kw, 4)  );     

      ecl_file_close( ecl_file_index );
      
   }
   test_work_area_free( work_area );
}


int main( int argc , char ** argv) {
   util_install_signals();
   test_load_nonexisting_file();
   test_create_and_load_index_file();
}
