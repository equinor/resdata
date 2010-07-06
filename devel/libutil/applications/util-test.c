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

int main(int argc , char ** argv) {
  printf("$HOME -> <%s> \n\n",util_alloc_envvar( "$HOME" ));
  printf("$WHAT -> <%s> \n\n",util_alloc_envvar( "$WHAT" ));
  printf("$HOME/BJARNE/$WHAT/$ERT_LD_PATH -> <%s>\n",util_alloc_envvar( "$HOME/BJARNE/$WHAT/$ERT_LD_PATH" ));
  printf("HEI -> <%s> \n",util_alloc_envvar( "HEI "));
}


