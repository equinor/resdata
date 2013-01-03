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
  if (s1 == NULL && s2 == NULL)
    return true;
  else {
    if (s1 == NULL)
      return false;
    if (s2 == NULL)
      return false;

    if (strcmp(s1,s2) == 0)
      return true;
    else
      return false;
  }
}
