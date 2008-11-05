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
#include <stringlist.h>
#include <menu.h>
#include <subst.h>





int main(int argc , char ** argv) {
  int * list;
  int   length;


  list = util_sscanf_alloc_active_list("1,3,5-10,4-6,1-3" , &length);
  for (int i = 0; i < length; i++)
    printf("list[%d] = %d \n",i , list[i]);
}
