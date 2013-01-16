/*
   Copyright (C) 2012  Statoil ASA, Norway. 
   
   The file 'test_util.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#ifndef __TEST_UTIL_H__
#define __TEST_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

  void  test_error_exit( const char * fmt , ...);
  bool  test_string_equal( const char * s1 , const char * s2 );

  void  test_assert_int_equal( int i1 , int i2 , const char * fmt);
  void  test_assert_string_equal( const char * s1 , const char * s2 , const char * fmt);

  void test_assert_true( bool value, const char * fmt );
  void test_assert_false( bool value, const char * fmt );

  void test_assert_time_t_equal( time_t t1 , time_t t2 , const char * fmt);
  void test_assert_time_t_not_equal( time_t t1 , time_t t2 , const char * fmt);
  

#ifdef __cplusplus
}
#endif
#endif
