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
  FILE * stream = util_fopen("test","r");
  char * buffer = util_fscanf_alloc_upto(stream , "Hove" , false);
  printf("[%s]" , buffer);
  free(buffer);
  fclose(stream);
}

