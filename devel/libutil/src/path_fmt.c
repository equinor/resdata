#include <stdlib.h>
#include <string.h>
#include <path_fmt.h>
#include <stdio.h>
#include <stdarg.h>
#include <util.h>
#include <stdbool.h>
#include <node_ctype.h>

struct path_fmt_struct {
  int   buffer_size;
  char *fmt;
  char *file_fmt;
  bool  is_directory;
  bool  auto_mkdir;
};




void path_fmt_reset_fmt(path_fmt_type * path , const char * fmt) {
  path->fmt = util_realloc_string_copy(path->fmt , fmt);
  if (path->is_directory) 
    path->file_fmt = util_alloc_string_sum((const char *[2]) {path->fmt , "/%s"} , 2);
}



static path_fmt_type * path_fmt_alloc__(const char * fmt , bool is_directory , bool auto_mkdir) {
  path_fmt_type * path = util_malloc(sizeof * path , __func__);
  path->fmt          = NULL;
  path->file_fmt     = NULL;
  path->is_directory = is_directory;
  path->auto_mkdir   = auto_mkdir;
  
  path_fmt_reset_fmt(path , fmt);
  return path;
}

/**

  This function is used to allocate a path_fmt instance which is
  intended to hold a directory, if the second argument is true, the
  resulting directory will be automatically created when
  path_fmt_alloc_path() is later invoked.

  Example:
  -------
  path_fmt_type * path_fmt = path_fmt_alloc_directory_fmt("/tmp/scratch/member%d/%d.%d" , true);
  ....
  ....
  char * path = path_fmt_alloc_path(path_fmt , 10 , 12 , 15);
  char * file = path_fmt_alloc_file(path_fmt ,  8 , 12 , 17, "SomeFile");

  After the two last function calls we will have:

   o path = "/tmp/scratch/member10/12.15" - and this directory has
     been created. 

   o file = "/tmp/scratch/member8/12.17/SomeFile - and the directory
     /tmp/scratch/member8/12.17 has been created.

     
  Observe that the functionality is implemented with the help av
  variable length argument lists, and **NO** checking of argument list
  versus format string is performed.
*/

path_fmt_type * path_fmt_alloc_directory_fmt(const char * fmt , bool auto_mkdir) {
  return path_fmt_alloc__(fmt , true , auto_mkdir);
}


path_fmt_type * path_fmt_alloc_file_fmt(const char * fmt) {
  return path_fmt_alloc__(fmt , false , false);
}


path_fmt_type * path_fmt_copyc(const path_fmt_type *path) {
  path_fmt_type *new_path = path_fmt_alloc__(path->fmt , path->is_directory , path->auto_mkdir);
  return new_path;
}


static char * __fmt_alloc_path_va__(const char * fmt , va_list ap) {
  char * new_path;
  int path_length;
  {
    va_list tmp_va;
    va_copy(tmp_va , ap);
    path_length = vsnprintf(new_path , 0 , fmt , tmp_va);
  }

  new_path = malloc(path_length + 1);
  vsnprintf(new_path , path_length + 1 , fmt , ap);
  return new_path;
}


char * path_fmt_alloc_path_va(const path_fmt_type * path , va_list ap) {
  char * new_path = __fmt_alloc_path_va__(path->fmt , ap);
  if (path->auto_mkdir)
    util_make_path(new_path);
  return new_path;
}


char * path_fmt_alloc_path(const path_fmt_type * path , ...) {
  char * new_path;
  va_list ap;
  va_start(ap , path);
  new_path = path_fmt_alloc_path_va(path , ap);
  va_end(ap);
  return new_path;
}



char * path_fmt_alloc_file(const path_fmt_type * path , ...) {
  if (path->is_directory) {
    char * filename;
    va_list ap;
    va_start(ap , path);
    filename = __fmt_alloc_path_va__(path->file_fmt , ap);
    if (path->auto_mkdir) {
      if (! util_file_exists(filename)) {
	const char * __path = __fmt_alloc_path_va__(path->fmt , ap);
	util_make_path(__path);
	free((char *) __path);
      }
    }
    va_end(ap);
    return filename;
  } else 
    util_abort("%s: tried to allocate filename from a path_fmt object which already is of file type - aborting\n",__func__);
}

/**
   This function is used to assert that the format in a path_fmt
   instance is according to specification. What is checked is that the
   format string contains %d, %lfg and %s in as specified in the
   input_types vector.

   Observe that %s is mapped to void_pointer - as the node_ctype does not
   have typed pointers.
*/


/*
void path_fmt_assert_fmt(const path_fmt_type * path , int num_input , const node_ctype * input_types) {
  int input_nr = 0;
  int char_nr  = 0;
  do {
    if (path->fmt[char_nr] == '%') {

    }
  }
}
*/

const char * path_fmt_get_fmt(const path_fmt_type * path) {
  return path->fmt;
}


void path_fmt_free(path_fmt_type * path) {
  free(path->fmt);
  if (path->is_directory)
    free(path->file_fmt);
  free(path);
}
