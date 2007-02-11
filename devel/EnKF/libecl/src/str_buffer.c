#include <string.h>
#include <str_buffer.h>

struct str_buffer_struct {
  int   size;
  int   len;
  char *buffer;
}; 



static void str_buffer_realloc(str_buffer_type *str_buffer , int new_size) {
  str_buffer->size   = new_size;
  str_buffer->buffer = realloc(str_buffer->buffer , new_size * (sizeof *str_buffer->buffer));
}

static void str_buffer_grow(str_buffer_type *str_buffer) { 
  str_buffer_realloc(str_buffer , str_buffer->size * 2); 
}

static void __str_buffer_fprintf_substring(str_buffer_type *str_buffer, int i1, int i2, FILE *stream) {
  if (i1 < 0 || i2 > str_buffer->len) {
    fprintf(stderr,"%s: substring interval : [%d,%d> invalid - aborting \n",__func__ , i1 , i2);
    abort();
  }
  {
    int i;
    for (i=0; i < (i2 - i1); i++)
      fprintf(stream,"%c",str_buffer->buffer[i]);
  }
}


str_buffer_type * str_buffer_alloc(int size) {
  str_buffer_type *str_buffer = malloc(sizeof *str_buffer);
  str_buffer->buffer = NULL;
  str_buffer_realloc(str_buffer , size);
  str_buffer->len = 0;
  return str_buffer;
}


void str_buffer_free(str_buffer_type *str_buffer) {
  free(str_buffer->buffer);
  free(str_buffer);
}


void str_buffer_add_string(str_buffer_type *str_buffer , const char *s) {
  if (strlen(s) + str_buffer->len >= str_buffer->size)
    str_buffer_grow(str_buffer);
  strcat(str_buffer->buffer , s);
  str_buffer->len += strlen(s);
}


void str_buffer_fprintf_substring(str_buffer_type *str_buffer , int i1 , int i2 , FILE *stream) {
  if (i2 < 0) 
    __str_buffer_fprintf_substring(str_buffer , i1 , str_buffer->len + i2 , stream);
  else
    __str_buffer_fprintf_substring(str_buffer , i1 , i2 , stream);
}

const char * str_buffer_get_char_ptr(const str_buffer_type *str_buffer) {
  return str_buffer->buffer;
}
