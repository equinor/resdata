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
#include <matrix.h>
#include <matrix_lapack.h>
#include <matrix_blas.h>
#include <conf.h>


void test(const char *s , const char * p) {
  if (util_string_match(s , p))
    printf("\"%s\" matches \"%s\" \n",s,p);
  else
    printf("\"%s\" does NOT match \"%s\" \n",s,p);
}


int main(int argc , char ** argv) {
  test("Joakim Hove" , "Joakim*");
}

