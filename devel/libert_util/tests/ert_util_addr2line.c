/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'ert_util_addr2line.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <execinfo.h>

#include <ert/util/test_util.h>
#include <ert/util/stringlist.h>
#include <ert/util/util.h>


void test_lookup() {
  const char * file = __FILE__;
  const char * func = __func__;
  const int    line = __LINE__;
  const int max_bt = 50;
  void *bt_addr[max_bt];  
  int size = backtrace(bt_addr , max_bt);    
  char * func_name , * file_name;
  int line_nr;
  
  test_assert_int_equal( size , 4 );
  test_assert_true( util_addr2line_lookup( NULL , bt_addr[0] , NULL , &func_name , &file_name , &line_nr));
  test_assert_string_equal( func_name , func );
  test_assert_int_equal( line + 1 , line_nr );
  test_assert_string_equal( file_name , file );
}


int main( int argc , char ** argv) {
  test_lookup();
  exit(0);
}
