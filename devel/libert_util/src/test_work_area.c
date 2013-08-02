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

#define DEFAULT_PREFIX "/tmp/ert-test"
#define PATH_FMT       "%s/%s/%s"     /* PREFIX / username / test_name */

struct test_work_area_struct {
  bool        store;
  char      * cwd;
  char      * original_cwd;
};



test_work_area_type * test_work_area_alloc_with_prefix(const char * prefix , const char * test_name, bool store) {
  if (test_name) {
    test_work_area_type * work_area = util_malloc( sizeof * work_area );
    work_area->store = store;
    {
      uid_t uid = getuid();
      struct passwd * pw = getpwuid( uid );
      
      work_area->cwd = util_alloc_sprintf( PATH_FMT , prefix , pw->pw_name , test_name );
    }
    work_area->original_cwd = util_alloc_cwd();
    util_make_path( work_area->cwd );
    chdir( work_area->cwd );
    return work_area;
  } else 
    return NULL;
}

test_work_area_type * test_work_area_alloc(const char * test_name, bool store) {
  return test_work_area_alloc_with_prefix( DEFAULT_PREFIX , test_name , store );
}


void test_work_area_free(test_work_area_type * work_area) { 
  if (!work_area->store)
    util_clear_directory( work_area->cwd , true , true );

  chdir( work_area->original_cwd );
  free( work_area->original_cwd );
  free( work_area->cwd );
  free( work_area );
}


const char * test_work_area_get_cwd( const test_work_area_type * work_area ) {
  return work_area->cwd;
}



void test_work_area_install_file( test_work_area_type * work_area , const char * input_src_file ) {
  if (util_is_abs_path( input_src_file ))
    return;
  else {
    char * src_file = util_alloc_filename( work_area->original_cwd , input_src_file , NULL );
    char * src_path;
    
    util_alloc_file_components( input_src_file , &src_path , NULL , NULL);
    if (!util_entry_exists( src_path ))
      util_make_path( src_path );

    if (util_file_exists( src_file )) {
      char * target_file   = util_alloc_filename( work_area->cwd , input_src_file , NULL );
      util_copy_file( src_file , target_file );
      free( target_file );
    }
    free( src_file );
  }
}


void test_work_area_copy_directory( test_work_area_type * work_area , const char * input_directory) {
  char * src_directory;

  if (util_is_abs_path( input_directory ))
    src_directory = util_alloc_string_copy( input_directory );
  else
    src_directory = util_alloc_filename( work_area->original_cwd , input_directory , NULL);

  util_copy_directory(src_directory , work_area->cwd );
  free( src_directory );
}


void test_work_area_copy_directory_content( test_work_area_type * work_area , const char * input_directory) {
  char * src_directory;

  if (util_is_abs_path( input_directory ))
    src_directory = util_alloc_string_copy( input_directory );
  else
    src_directory = util_alloc_filename( work_area->original_cwd , input_directory , NULL);

  util_copy_directory_content(src_directory , work_area->cwd );
  free( src_directory );
}




