#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>

int main(int argc , char ** argv) {
  util_fprintf_double(1.78       , 10 , 6 , stdout);
  printf("\n");
  util_fprintf_double(3.14159265 , 12 , 6 , stdout);
  printf("\n");
  util_fprintf_string("Hei Joakim" , 50, true , stdout);
  printf("\n");
  util_fprintf_string("Hei Joakim" , 50, false , stdout);
  printf("\n");
}
