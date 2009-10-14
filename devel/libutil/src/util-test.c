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





int main(int argc , char ** argv) {
  size_t compressed_size = 80;
  double x  = 189;
  void * zx = util_malloc( compressed_size , __func__);
  util_compress_buffer( &x , 8 , zx , &compressed_size );
  printf("Compress OK ?? compressed_size:%d \n" , compressed_size);
}


