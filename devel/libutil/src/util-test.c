#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


int main (int argc , char **argv) {
  int buffer_size;
  char * buffer = util_fread_alloc_file_content("test.txt" , "--" , &buffer_size);
  printf("%s",buffer);
  util_abort("%s: naa er det nok:%s:%d\n",__func__ , __FILE__ , __LINE__);
  free(buffer);
}
