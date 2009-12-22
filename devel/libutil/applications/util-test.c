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
  
  float  float_value  = 7882192.737;
  double double_value =  7882192.737;
  const char * s = "0.24690000000000D+06";
  double arg;
  int c , int_power;
  
  c = sscanf(s , "%lgD%d" , &arg , &int_power);
  printf("c:%d  arg:%g   power:%d \n",c , arg , int_power);
}


