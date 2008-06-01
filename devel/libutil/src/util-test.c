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



int main(int argc , char ** argv) {
  util_vfork_exec("./test.py" , 0 , NULL , true , "test.out" , NULL , "f.stdout" , "f.stderr");
  printf("-----------------------------------------------------------------\n");
  util_vfork_exec("./test.py" , 0 , NULL , true , "test.out" , NULL , NULL , NULL);
}
