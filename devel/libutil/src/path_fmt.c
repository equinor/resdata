#include <stdlib.h>
#include <string.h>
#include <path_fmt.h>
#include <stdio.h>
#include <stdarg.h>
#include <util.h>
#include <stdbool.h>

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
  int path_length = vsnprintf(new_path , 0 , fmt , ap);
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
  } else {
    fprintf(stderr,"%s: tried to allocate filename from a path_fmt object which already is of file type - aborting\n",__func__);
    abort();
  }
}


const char * path_fmt_get_fmt(const path_fmt_type * path) {
  return path->fmt;
}


void path_fmt_free(path_fmt_type * path) {
  free(path->fmt);
  free(path);
}
