#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <config.h> 
#include <stringlist.h>
#include <menu.h>





int main(int argc , char ** argv) {
  size_t value;

  if (util_sscanf_bytesize(argv[1] , &value))
    printf("%s -> %d \n",argv[1],value);
  else
    printf("Failed to parse:%s \n",argv[1]);
}
