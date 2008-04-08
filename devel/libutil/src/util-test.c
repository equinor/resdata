#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


int main (int argc , char **argv) {
  printf("util_path_exist() : %d \n",util_path_exists("/tmp/Funny/Path"));
  printf("util_file_exist() : %d \n",util_file_exists("/tmp/Funny/Path"));
  printf("util_path_exist() : %d \n",util_path_exists("/tmp/file"));
  printf("util_file_exist() : %d \n",util_file_exists("/tmp/file"));
}
