#include <stdlib.h>
#include <signal.h>
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
#include <block_fs.h>
#include <thread_pool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




int main(int argc , char ** argv) {
  char * filename = "/tmp/kast.txt";
  int    fd      = open( filename , O_CREAT );
  FILE * stream;

  printf("fd:%d -> ",fd);
  fd = dup2(1 , fd);
  printf("%d \n",fd);

  stream = fdopen( fd , "w");
  fprintf(stream , "Hei ..... \n");
  fclose(stream);
}


