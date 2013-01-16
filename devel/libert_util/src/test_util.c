/*
   Copyright (C) 2012  Statoil ASA, Norway. 
   
   The file 'test_util.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <util.h>
#include <test_util.h>

void test_error_exit( const char * fmt , ...) {
  char * s;
  va_list ap;
  va_start(ap , fmt);
  s = util_alloc_sprintf_va(fmt , ap);
  va_end(ap);

  fprintf( stderr , s );
  exit(1);
}


bool test_string_equal( const char * s1 , const char * s2 ) {
  bool equal = true;
  if (s1 == NULL && s2 == NULL)
    return true;
  else {
    if (s1 == NULL)
      equal = false;
    if (s2 == NULL)
      equal = false;

    if (strcmp(s1,s2) == 0)
      return true;
    else
      equal = false;
  }
  return equal;
}



void test_assert_string_equal( const char * s1 , const char * s2 , const char * fmt) {
  bool equal = test_string_equal( s1 , s2 );
  if (!equal) {
    if (fmt == NULL)
      test_error_exit( "String are different s1:[%s]  s2:[%s]\n" , s1 , s2 );
    else
      test_error_exit( fmt , s1 , s2 );
  }
}


void test_assert_int_equal( int i1 , int i2 , const char * fmt) {
  if (i1 != i2) {
    if (fmt == NULL)
      test_error_exit( "Integers are different i1:[%d]  i2:[%d]\n" , i1 , i2 );
    else
      test_error_exit( fmt , i1 , i2 );
  }
}

/*****************************************************************/

void test_assert_time_t_equal( time_t t1 , time_t t2 , const char * fmt) {
  if (t1 != t2) {
    if (fmt == NULL)
      test_error_exit("time_t values are different t1:%d  t2:[%d]" , t1 , t2);
    else
      test_error_exit( fmt , t1 , t2 );
  }
}


void test_assert_time_t_not_equal( time_t t1 , time_t t2 , const char * fmt) {
  if (t1 == t2) {
    if (fmt == NULL)
      test_error_exit("time_t values are different t1:%d  t2:[%d]" , t1 , t2);
    else
      test_error_exit( fmt , t1 , t2 );
  }
}

/*****************************************************************/

void test_assert_true( bool value, const char * fmt ) {
  if (!value) {
    if (fmt == NULL)
      test_error_exit("assert( true ) failed");
    else
      test_error_exit(fmt);
  }
}


void test_assert_false( bool value, const char * fmt ) {
  if (value) {
    if (fmt == NULL)
      test_error_exit("assert( false ) failed");
    else
      test_error_exit(fmt);
  }
}
