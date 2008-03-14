#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


int main (int argc , char **argv) {
  char * s = util_alloc_joined_string((const char *[4]) {"Hei","Joakim Hove" ,"Henrik Mohnsvei 6" , "5067 Bergen"} , 4 , "<X>");
  printf("s: %s \n",s);
}
