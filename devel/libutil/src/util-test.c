#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <void_arg.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>


void func2(int arg1 , int arg2) {
  util_abort("%s: gir opp ... \n",__func__);
}

void func1(int arg1 , int arg2) {
  func2(arg1 , arg2 + arg1);
}


int main (int argc , char **argv) {
  char * cwd = util_alloc_cwd();
  printf("Jeg er i %s \n",cwd);
  func1(10 , 11);
}
