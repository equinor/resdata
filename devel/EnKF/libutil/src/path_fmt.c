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
  char *path;
  bool  path_set;
};


static void path_fmt_realloc(path_fmt_type * path, int new_buffer_size) {
  path->path = realloc(path->path , new_buffer_size + 1); /* One extra to include \0 */
  path->buffer_size = new_buffer_size;
}


void path_fmt_reset_fmt(path_fmt_type * path , const char * fmt) {
  path->fmt = util_realloc_string_copy(path->fmt , fmt);
  path->path_set = false;
}


path_fmt_type * path_fmt_alloc(const char * fmt) {
  path_fmt_type * path = util_malloc(sizeof * path , __func__);
  path->fmt         = NULL;
  path->path        = NULL;
  
  path_fmt_reset_fmt(path , fmt);
  path_fmt_realloc(path , 2 * strlen(path->fmt));
  return path;
}



void path_fmt_set_va(path_fmt_type * path , va_list ap) {
  int path_length;
  path_length = vsnprintf(path->path , path->buffer_size , path->fmt , ap);
  if (path_length >= path->buffer_size) {
    path_fmt_realloc(path , path_length + 10);
    vsnprintf(path->path , path->buffer_size , path->fmt , ap);
  }
  path->path_set = true;
  util_make_path(path->path);
}


void path_fmt_set(path_fmt_type * path , ...) {
  va_list ap;

  va_start(ap , path->fmt);
  path_fmt_set_va(path , ap);
  va_end(ap);
  
}



const char * path_fmt_get_path(const path_fmt_type * path) {
  if (path->path_set)
    return path->path;
  else {
    fprintf(stderr,"%s: must call path_fmt_set() first - aborting \n",__func__);
    abort();
  }
}


const char * path_fmt_get_fmt(const path_fmt_type * path) {
  return path->fmt;
}


void path_fmt_free(path_fmt_type * path) {
  free(path->path);
  free(path->fmt);
  free(path);
}
