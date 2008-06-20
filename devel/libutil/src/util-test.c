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


int main(int argc , char ** argv) {
  
  printf("OK - I am aborting \n");
  util_abort("%s: failing \n",__func__);
  
}
