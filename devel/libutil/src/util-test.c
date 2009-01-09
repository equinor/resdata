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




int main(int argc , char ** argv) {
  double data[3] = {1.0, 2.0, 3.0};

  printf("mean  : %f\n", util_double_vector_mean(3, data));
  printf("stddev: %f\n", util_double_vector_stddev(3, data));

  return 0;
}
