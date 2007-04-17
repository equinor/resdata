#include <string.h>
#include <str_buffer.h>

#define NULL_CHAR  '\0'
struct str_buffer_struct {
  int   size;
  int   len;
  char *buffer;
}; 



static void str_buffer_realloc(str_buffer_type *str_buffer , int new_size) {
  str_buffer->size   = new_size;
  str_buffer->buffer = realloc(str_buffer->buffer , new_size * (sizeof *str_buffer->buffer));
}

static void str_buffer_grow(str_buffer_type *str_buffer , int str_size) { 
  const int old_size = str_buffer->size;
  const int min_size = str_buffer->len + str_size + 1;
  int  new_size;
  
  if (old_size * 2 >= min_size)
    new_size = 2 * old_size;
  else
    new_size = old_size + 2*str_size;

  str_buffer_realloc(str_buffer , new_size); 
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
  str_buffer->buffer[0] = NULL_CHAR;
  return str_buffer;
}

str_buffer_type * str_buffer_alloc_with_string(const char *s) {
  str_buffer_type *str_buffer = str_buffer_alloc(2 * strlen(s));
  str_buffer_add_string(str_buffer , s);
  return str_buffer;
}


void str_buffer_free(str_buffer_type *str_buffer) {
  free(str_buffer->buffer);
  free(str_buffer);
}


/*
  static bool str_buffer_large_enough(const str_buffer_type , 
*/


static void str_buffer_add_char_vector(str_buffer_type *str_buffer , const char *c , int len) {
  if (len + str_buffer->len >= (str_buffer->size - 1))
    str_buffer_grow(str_buffer , len);
  {
    int i;
    for (i=0; i < len; i++)
      str_buffer->buffer[i+str_buffer->len] = c[i];
    str_buffer->len += len;
    str_buffer->buffer[str_buffer->len] = NULL_CHAR;
  }
}


void str_buffer_add_string(str_buffer_type *str_buffer , const char *s) {
  str_buffer_add_char_vector(str_buffer , s , strlen(s));
}


void str_buffer_add_char(str_buffer_type *str_buffer , char c) {
  str_buffer_add_char_vector(str_buffer , &c , 1);
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

#undef NULL_CHAR
