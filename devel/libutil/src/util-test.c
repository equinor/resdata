#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>
#include <arg_pack.h>
#include <vector.h>
#include <double_vector.h>
#include <matrix.h>
#include <matrix_lapack.h>
#include <matrix_blas.h>
#include <parser.h>



int main(int argc , char ** argv) {
  parser_type * parser = parser_alloc(NULL , "\"\'" , NULL , "\n" , "/*" , "*/");
  char * buffer = util_fread_alloc_file_content("test" ,  NULL);
  printf("-----------------------------------------------------------------\n");
  printf("%s \n",buffer);
  printf("-----------------------------------------------------------------\n");
  parser_strip_buffer( parser , &buffer );
  printf("-----------------------------------------------------------------\n");
  printf("%s \n",buffer);
  printf("-----------------------------------------------------------------\n");
  
}
  

