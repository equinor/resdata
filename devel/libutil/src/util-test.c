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
#include <conf.h>
#include <tokenizer.h>



int main(int argc , char ** argv) {
  tokenizer_type * tokenizer = tokenizer_alloc(NULL , NULL , ",-" , " \n\t\r" , NULL , NULL);
  stringlist_type * numbers = tokenize_file( tokenizer , "test" , false);
  stringlist_fprintf(numbers , " " , stdout);
}

