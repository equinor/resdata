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


int main(int argc , char ** argv) {
  mzran_type * rng = mzran_alloc( INIT_NONE );
  printf("rng 1:%d  2:%d \n",mzran_get_int( rng ) , mzran_get_int( rng ));
  mzran_init( rng , INIT_DEV_RANDOM );
  printf("rng 1:%d  2:%d \n",mzran_get_int( rng ) , mzran_get_int( rng ));
}


