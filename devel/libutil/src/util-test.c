#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


int main (int argc , char **argv) {
  char *s1 = "  Hei  ";
  char *s2 = "Joakim Hove";
  char *s3 = "            ";
  char *s4 = "Test1  ";

  printf("<%s> -> <%s> \n",s1 , util_alloc_strip_copy(s1));
  printf("<%s> -> <%s> \n",s2 , util_alloc_strip_copy(s2));
  printf("<%s> -> <%s> \n",s3 , util_alloc_strip_copy(s3));
  printf("<%s> -> <%s> \n",s4 , util_alloc_strip_copy(s4));
}
