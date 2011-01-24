#include <stdlib.h>
#include <math.h>
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
#include <lookup_table.h>


void test( const char * s1 , const char *s2) {
  int cmp = util_strcmp_float( s1 , s2 );
  int scmp = strcmp( s1 , s2 );
  printf(" cmp(%s,%s) = %d   strcmp(%s,%s) = %d\n",s1,s2,cmp,s1,s2,scmp);
}

int main(int argc , char ** argv) {
  test("1.00123000","1.00123");
}


