#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>





int main(int argc , char ** argv) {
  int    buffer_size;
  char * buffer      = util_fread_alloc_file_content("/h/a152128/EnKF/devel/EnKF/libenkf/src/field.c" , NULL , &buffer_size);
  
  
  util_string_replace_inplace(&buffer , &buffer_size , "field" , "FIELD");
  util_string_replace_inplace(&buffer , &buffer_size , "util" , "UT");
  util_string_replace_inplace(&buffer , &buffer_size , "config" , "configXX");
  {
    FILE * stream = util_fopen("kast.c" , "w");
    fwrite( buffer , 1 , strlen(buffer) , stream);
    fprintf(stream , "\n");
    fclose( stream );
  }
  free(buffer);
}
