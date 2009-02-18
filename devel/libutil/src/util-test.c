#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <string.h>      
#include <path_fmt.h>
#include <stdarg.h>
#include <hash.h>
#include <unistd.h>
#include <thread_pool.h>
#include <stringlist.h>
#include <menu.h>
#include <subst.h>
#include <arg_pack.h>
#include <vector.h>
#include <double_vector.h>


void test_split(const char *s, const char * sep, bool split_on_first) {
  char * s1 , *s2;
  util_binary_split_string(s , sep , split_on_first , &s1 , &s2);
  printf("%s -> <%s> + <%s> \n",s , s1 , s2);
  util_safe_free(s1);
  util_safe_free(s2);
  
}



int main(int argc , char ** argv) {
  test_split(":Navn" , ":" , false);
  test_split(":Navn:" , ":" , true);
}
