/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'ecl_nnc_export.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/util/test_util.h>
#include <ert/util/time_t_vector.h>
#include <ert/util/util.h>

#include <ert/ecl/ecl_util.h>
#include <ert/ecl/ecl_nnc_export.h>
#include <ert/ecl/ecl_kw_magic.h>

int count_kw_data( const ecl_file_type * file , ecl_grid_type * grid , const char * kw ) {
  int i,j;
  int count = 0;

  for (i=0; i < ecl_file_get_num_named_kw( file , kw ); i++) {
    ecl_kw_type * ecl_kw = ecl_file_iget_named_kw( file , kw , i );
    ecl_grid_type * igrid;
    
    if (i == 0)
      igrid= grid;
    else
      igrid = ecl_grid_iget_lgr( grid , i );
    {
      int global_size = ecl_grid_get_global_size( grid );
      for (j=0; j < ecl_kw_get_size( ecl_kw ); j++) {
        if (ecl_kw_iget_int( ecl_kw , j ) <= global_size)
          count +=1;
      }
    }
  }
  return count;
}



void test_count(const char * name) {
  char * grid_file_name = ecl_util_alloc_filename(NULL , name , ECL_EGRID_FILE , false  , -1);
  ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
  ecl_file_type * grid_file = ecl_file_open( grid_file_name , 0 );
  
  int num_nnc = 0;

  num_nnc  = count_kw_data( grid_file , grid , "NNC1" );
  num_nnc += count_kw_data( grid_file , grid , "NNCG" );
  num_nnc += count_kw_data( grid_file , grid , "NNA1");
  
  test_assert_int_equal( num_nnc , ecl_nnc_export_get_size( grid ));

  free(grid_file_name);
  ecl_grid_free( grid );
  ecl_file_close( grid_file );
}




void test_export(const char * name) {
  char * grid_file_name = ecl_util_alloc_filename(NULL , name , ECL_EGRID_FILE , false  , -1);
  char * init_file_name = ecl_util_alloc_filename(NULL , name , ECL_INIT_FILE , false  , -1);  
  ecl_grid_type * grid = ecl_grid_alloc( grid_file_name );
  ecl_file_type * grid_file = ecl_file_open( grid_file_name , 0 );
  ecl_file_type * init_file = ecl_file_open( init_file_name , 0);
  ecl_nnc_type * nnc_data1 = util_calloc( ecl_nnc_export_get_size( grid ) , sizeof * nnc_data1 );
  ecl_nnc_type * nnc_data2 = util_calloc( ecl_nnc_export_get_size( grid ) , sizeof * nnc_data2 );

  {
    int nnc_offset = 0;
    int block_nr = 0;
    for (block_nr = 0; block_nr < ecl_file_get_num_named_kw( grid_file , NNCHEAD_KW); block_nr++) {
      ecl_file_push_block( init_file );
      ecl_file_push_block( grid_file );
      ecl_file_select_block( grid_file , NNCHEAD_KW , block_nr );

      // This is too simple
      if (ecl_file_has_kw( init_file , TRANLL_KW))
        ecl_file_select_block( init_file , TRANLL_KW , block_nr );
      else
        ecl_file_select_block( init_file , TRANNNC_KW , block_nr );
      
      if (ecl_file_has_kw( grid_file , NNC1_KW )) {
        ecl_kw_type * nnc1_kw  = ecl_file_iget_named_kw( grid_file , NNC1_KW , 0 );
        ecl_kw_type * nnc2_kw  = ecl_file_iget_named_kw( grid_file , NNC2_KW , 0 );
        ecl_kw_type * nnchead  = ecl_file_iget_named_kw( grid_file , NNCHEAD_KW , 0);
        ecl_kw_type * nnc_tran = ecl_file_iget_named_kw( init_file , TRANNNC_KW , 0 );
        int i;

        test_assert_int_equal( ecl_kw_get_size( nnc1_kw ) , ecl_kw_get_size( nnc_tran ));
        for (i=0; i < ecl_kw_get_size( nnc1_kw); i++) {
          nnc_data1[ i + nnc_offset ].grid_nr1 = ecl_kw_iget_int( nnchead , NNCHEAD_LGR_INDEX);
          nnc_data1[ i + nnc_offset ].grid_nr2 = ecl_kw_iget_int( nnchead , NNCHEAD_LGR_INDEX);
          nnc_data1[ i + nnc_offset ].global_index1 = ecl_kw_iget_int( nnc1_kw , i) - 1;
          nnc_data1[ i + nnc_offset ].global_index2 = ecl_kw_iget_int( nnc2_kw , i) - 1;
          nnc_data1[ i + nnc_offset ].trans = ecl_kw_iget_as_double( nnc_tran , i );
        }
        nnc_offset += ecl_kw_get_size( nnc1_kw );
      }
      
      
      if (ecl_file_has_kw( grid_file, NNCL_KW)) {
        ecl_kw_type * nncl_kw  = ecl_file_iget_named_kw( grid_file , NNCL_KW , 0 );
        ecl_kw_type * nncg_kw  = ecl_file_iget_named_kw( grid_file , NNCG_KW , 0 );
        ecl_kw_type * nnchead  = ecl_file_iget_named_kw( grid_file , NNCHEAD_KW , 0);
        ecl_kw_type * nnc_tran = ecl_file_iget_named_kw( init_file , TRANGL_KW , 0 );
        int i;
        
        test_assert_int_equal( ecl_kw_get_size( nncl_kw ) , ecl_kw_get_size( nnc_tran ));
        for (i=0; i < ecl_kw_get_size( nncl_kw); i++) {
          nnc_data1[ i + nnc_offset ].grid_nr1 = 0;
          nnc_data1[ i + nnc_offset ].grid_nr2 = ecl_kw_iget_int( nnchead , NNCHEAD_LGR_INDEX);
          nnc_data1[ i + nnc_offset ].global_index1 = ecl_kw_iget_int( nncg_kw , i) - 1;
          nnc_data1[ i + nnc_offset ].global_index2 = ecl_kw_iget_int( nncl_kw , i) - 1;
          nnc_data1[ i + nnc_offset ].trans = ecl_kw_iget_as_double( nnc_tran , i );
        }
        nnc_offset += ecl_kw_get_size( nncl_kw );
      }
      
      if (ecl_file_has_kw( grid_file , NNA1_KW )) {
        ecl_kw_type * nnc1_kw = ecl_file_iget_named_kw( grid_file , NNA1_KW , block_nr );
        ecl_kw_type * nnc2_kw = ecl_file_iget_named_kw( grid_file , NNA2_KW , block_nr );
        ecl_kw_type * nnchead = ecl_file_iget_named_kw( grid_file , NNCHEADA_KW , block_nr);
        ecl_kw_type * nnc_tran = ecl_file_iget_named_kw( init_file , TRANLL_KW , 0 );
        int i;

        test_assert_int_equal( ecl_kw_get_size( nnc1_kw ) , ecl_kw_get_size( nnc_tran ) );
        for (i=0; i < ecl_kw_get_size( nnc1_kw); i++) {
          nnc_data1[ i + nnc_offset ].grid_nr1 = ecl_kw_iget_int( nnchead , NNCHEADA_ILOC1_INDEX );
          nnc_data1[ i + nnc_offset ].grid_nr2 = ecl_kw_iget_int( nnchead , NNCHEADA_ILOC1_INDEX );
          nnc_data1[ i + nnc_offset ].global_index1 = ecl_kw_iget_int( nnc1_kw , i) - 1;
          nnc_data1[ i + nnc_offset ].global_index2 = ecl_kw_iget_int( nnc2_kw , i) - 1;
          nnc_data1[ i + nnc_offset ].trans = ecl_kw_iget_as_double( nnc_tran , i );;
        }
        nnc_offset += ecl_kw_get_size( nnc1_kw );
      }
      ecl_file_pop_block( grid_file );
      ecl_file_pop_block( init_file );
    }
   
    
    ecl_nnc_sort( nnc_data1 , nnc_offset );
  }
  
  ecl_nnc_export( grid , init_file , nnc_data2 );
  if (1)
  {
    int i;
    for (i=0; i < ecl_nnc_export_get_size( grid ); i++) {
      if (ecl_nnc_cmp( &nnc_data1[i] , &nnc_data2[i])) 
        printf("Comparing: %d 1:(%d,%d)  2:(%d,%d)   1:(%ld,%ld)   2:(%ld,%ld) %g,%g\n",
             i , 
             nnc_data1[i].grid_nr1 , 
             nnc_data1[i].grid_nr2 , 
             nnc_data2[i].grid_nr1 , 
             nnc_data2[i].grid_nr2, 
             nnc_data1[i].global_index1 , 
             nnc_data1[i].global_index2 , 
             nnc_data2[i].global_index1 , 
             nnc_data2[i].global_index2, 
             nnc_data1[i].trans , 
             nnc_data2[i].trans);

      test_assert_int_equal( 0 , ecl_nnc_cmp( &nnc_data1[i] , &nnc_data2[i]));
    }
  }
  test_assert_int_equal( 0 , memcmp( nnc_data1 , nnc_data2 , ecl_nnc_export_get_size( grid ) * sizeof * nnc_data2 ));
  
  free( nnc_data2 );
  free( nnc_data1 );
  free(grid_file_name);
  ecl_grid_free( grid );
  ecl_file_close( grid_file );
  ecl_file_close( init_file );
}


void test_cmp() {

  ecl_nnc_type nnc1 = {.grid_nr1 = 1,
                       .grid_nr2 = 1,
                       .global_index1 = 1,
                       .global_index2 = 1 };

  ecl_nnc_type nnc2 = {.grid_nr1 = 1,   // nnc1 == nnc2
                       .grid_nr2 = 1,
                       .global_index1 = 1,
                       .global_index2 = 1 };

  ecl_nnc_type nnc3 = {.grid_nr1 = 4,   // nnc3 > nnc1
                       .grid_nr2 = 1,
                       .global_index1 = 1,
                       .global_index2 = 1 };
  

  ecl_nnc_type nnc4 = {.grid_nr1 = 4,   // nnc4 > nnc1
                       .grid_nr2 = 2,   // nnc4 > nnc2
                       .global_index1 = 1,
                       .global_index2 = 1 };
  
  ecl_nnc_type nnc5 = {.grid_nr1 = 4,   // nnc5 > nnc4
                       .grid_nr2 = 2,   
                       .global_index1 = 3,
                       .global_index2 = 1 };


  ecl_nnc_type nnc6 = {.grid_nr1 = 4,   // nnc6 > nnc5
                       .grid_nr2 = 2,   
                       .global_index1 = 3,
                       .global_index2 = 5 };

  
  test_assert_int_equal( 0 , ecl_nnc_cmp( &nnc1 , &nnc2 ));
  test_assert_int_equal( 1 , ecl_nnc_cmp( &nnc3 , &nnc1 ));
  test_assert_int_equal( 1 , ecl_nnc_cmp( &nnc4 , &nnc1 ));
  test_assert_int_equal( 1 , ecl_nnc_cmp( &nnc5 , &nnc4 ));
  test_assert_int_equal(-1 , ecl_nnc_cmp( &nnc4 , &nnc5 ));
  test_assert_int_equal(-1 , ecl_nnc_cmp( &nnc5 , &nnc6 ));
}


void test_sort() {
  const int N = 1000;
  ecl_nnc_type * nnc_list = util_calloc(N , sizeof( nnc_list ));
  int i;
  for (i=0; i < N; i++) {
    ecl_nnc_type nnc = {.grid_nr1 = (i % 19),
                        .grid_nr2 = (i % 3),   
                        .global_index1 = (i % 7),
                        .global_index2 = (i % 13)};
    nnc_list[i] = nnc;
  }
  ecl_nnc_sort( nnc_list , N );
  for (i=0; i < (N - 1); i++)
    test_assert_int_equal( -1 , ecl_nnc_cmp(&nnc_list[i] , &nnc_list[i+1]));
  
}


int main(int argc, char ** argv) {
  const char * base = argv[1];
  test_cmp( );
  test_sort();
  test_count( base );
  test_export( base );
  exit(0);
}
