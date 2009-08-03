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
#include <tokenizer.h>



int main(int argc , char ** argv) {
  tokenizer_type * tokenizer = tokenizer_alloc(NULL , "\"\'" , NULL , "\n" , "/*" , "*/");
  char * buffer = util_fread_alloc_file_content("test" ,  NULL);
  printf("-----------------------------------------------------------------\n");
  printf("%s \n",buffer);
  printf("-----------------------------------------------------------------\n");
  tokenizer_strip_buffer( tokenizer , &buffer );
  printf("-----------------------------------------------------------------\n");
  printf("%s \n",buffer);
  printf("-----------------------------------------------------------------\n");
  
}
  

