/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'path_stack.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <stringlist.h>
#include <util.h>
#include <path_stack.h>

struct path_stack_struct {
  stringlist_type * stack;
  stringlist_type * storage;
};



path_stack_type * path_stack_alloc() {
  path_stack_type * path_stack = util_malloc( sizeof * path_stack );
  path_stack->stack = stringlist_alloc_new();
  path_stack->storage = stringlist_alloc_new();
  return path_stack;
}


void path_stack_free( path_stack_type * path_stack ) {
  stringlist_free( path_stack->stack );
  stringlist_free( path_stack->storage );
}




bool path_stack_push( path_stack_type * path_stack , const char * path ) {
  if (path != NULL) {
    if (chdir( path ) != 0)
      return false;
  }
  path_stack_push_cwd( path_stack );
  return true;
}


void path_stack_push_cwd( path_stack_type * path_stack ) {
  char * cwd = util_alloc_cwd();
  stringlist_append_owned_ref( path_stack->storage , cwd);
  stringlist_append_ref( path_stack->stack , cwd );
}

const char * path_stack_pop( path_stack_type * path_stack ) {
  char * path = stringlist_pop( path_stack->stack );
  if (chdir( path ) == 0)
    return path;
  else {
    // The directory has become inaccesible ...
    util_abort("%s: could not pop back to directory:%s Error:%s\n", __func__ , path , strerror( errno ));
    return NULL;
  }
}


int path_stack_size( const path_stack_type * path_stack ) {
  return stringlist_get_size( path_stack->stack );
}


const char * path_stack_peek( const path_stack_type * path_stack ) {
  return stringlist_get_last( path_stack->stack );
}


