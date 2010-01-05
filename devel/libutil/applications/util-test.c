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


int main(int argc , char ** argv) {
  const char * s1 = "AAA";
  const char * s2 = "BBB";
  const char * s3 = "CCC";
  
  stringlist_type * slist = stringlist_alloc_new();
  stringlist_append_ref( slist , s1 );
  stringlist_append_ref( slist , s2 );
  stringlist_append_ref( slist , s3 );
  
  printf("s1: %p \n",s1);
  printf("s2: %p \n",s2);
  printf("s3: %p \n",s3);


  stringlist_sort( slist );
}


