/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
   The file 'test_work_area.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>


#include <ert/util/test_work_area.h>
#include <ert/util/util.h>

#define PATH_FMT "/tmp/ert-test/%s/%s"

struct test_work_area_struct {
  char * cwd;
  bool   store;
};



test_work_area_type * test_work_area_alloc(const char * test_name, bool store) {
  test_work_area_type * work_area = util_malloc( sizeof * work_area );
  work_area->store = store;
  {
    uid_t uid = getuid();
    struct passwd * pw = getpwuid( uid );
    
    work_area->cwd = util_alloc_sprintf( PATH_FMT , pw->pw_name , test_name );
  }
  util_make_path( work_area->cwd );
  chdir( work_area->cwd );
  return work_area;
}


void test_work_area_free(test_work_area_type * work_area) { 
  if (!work_area->store)
    util_clear_directory( work_area->cwd , true , true );
  
  free( work_area->cwd );
  free( work_area );
}


const char * test_work_area_get_cwd( const test_work_area_type * work_area ) {
  return work_area->cwd;
}


