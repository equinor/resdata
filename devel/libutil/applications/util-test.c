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
#include <subst_list.h>
#include <subst_func.h>
#include <buffer.h>
#include <mzran.h>
#include <statistics.h>
#include <double_vector.h>



int test( const char ** s) {
  int i    = 0;
  char * p = s[i];

  while (p != NULL) {
    printf("Looking at %s \n",p);
    i++;
    p = s[i];
  }
}

int main(int argc , char ** argv) {
  char ** token_list;
  int     len;

  test( (const char *[]) {"Hei","Joakim","GHove",NULL} );
  
}


