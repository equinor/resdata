#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


int main (int argc , char **argv) {
  char * cwd = util_alloc_cwd();
  printf("Jeg er i %s \n",cwd);
}
