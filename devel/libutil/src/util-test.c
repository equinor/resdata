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


void print1(void * _arg) {
  char * arg = (char *) _arg;
  printf("%s\n",arg);
}

void print2(void * _arg) {
  char * arg = (char *) _arg;
  printf("%s%s\n",arg,arg);
}

void print3(void * _arg) {
  char * arg = (char *) _arg;
  printf("%s%s%s\n",arg,arg,arg);
}



int main(int argc , char ** argv) {
  menu_type * menu = menu_alloc("Hovedmeny ..","qQ");
  menu_add_item(menu , "Print1" , "1a" , print1 , "Hei");
  menu_add_item(menu , "Print1" , "2bB" , print2 , "Hei");
  menu_add_item(menu , "Print1" , "3t" , print3 , "Hei");
  menu_run(menu);
  menu_free(menu);
}
