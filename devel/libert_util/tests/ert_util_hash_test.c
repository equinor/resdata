/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'ert_util_hash_test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <test_util.h>
#include <hash.h>

int main(int argc , char ** argv) {
  
  hash_type * h = hash_alloc();
  
  test_assert_ptr_equal( hash_add_option( h , "Key") , NULL , NULL);
  test_assert_string_equal( hash_add_option( h , "Key:Value" ) , "Value" , NULL );
  test_assert_string_equal( hash_add_option( h , "Key:Value2" ) , "Value2" , NULL );
  test_assert_string_equal( hash_add_option( h , "Key:Value2:Value3" ) , "Value2:Value3" , NULL );
  
  hash_free( h );
  exit(0);
}
