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




int main(int argc , char ** argv) {
  FILE * stream = util_fopen("util.c","r");
  while (util_fseek_string(stream , "seek")) {
    printf("Found seek at position: %d \n",ftell(stream));
    fseek( stream , 1 , SEEK_CUR);
  }
    
  fclose(stream);
}

