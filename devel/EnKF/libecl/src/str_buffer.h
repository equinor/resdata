#include <stdlib.h>
#include <stdio.h>

typedef struct str_buffer_struct str_buffer_type;


str_buffer_type * str_buffer_alloc(int );
void              str_buffer_free(str_buffer_type *);
void              str_buffer_add_string(str_buffer_type *, const char *);
void              str_buffer_fprintf_substring(str_buffer_type * , int , int , FILE *);
const char      * str_buffer_get_char_ptr(const str_buffer_type *);
