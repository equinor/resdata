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
  time_t t =  util_make_datetime(0 , 0 , 15, 1,7,2000);
  util_inplace_forward_days(&t , 180);
}

