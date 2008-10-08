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
  /*char * buffer      = util_fread_alloc_file_content("input" , NULL , &buffer_size);*/
  char * buffer      = util_alloc_sprintf("XXXXXXXXXXXXX");
  buffer_size = strlen(buffer + 1);
  
  
  util_string_replace_inplace(&buffer , &buffer_size , "X" , "GGGGGGGGGGGGG");
  util_string_replace_inplace(&buffer , &buffer_size , "K" , "J");
  {
    FILE * stream = util_fopen("/tmp/kast.txt" , "w");
    fwrite( buffer , 1 , strlen(buffer) , stream);
    fclose( stream );
  }
  free(buffer);
}
