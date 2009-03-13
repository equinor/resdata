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



void test(char ** ptr) {
  double value;
  util_fread_from_buffer( &value , sizeof value , 1 , ptr); printf("Have read:%g \n",value);
}


int main(int argc , char ** argv) {
  double * buffer = util_malloc( 3 * sizeof * buffer , __func__);
  util_copy_directory("/h/joaho/EnKF/devel/EnKF/libutil" , "/tmp/EnKF/devel");

  buffer[0] = 123;
  buffer[1] = 456;
  buffer[2] = 789;

  {
    double value;
    char * ptr = (char *) buffer;
    test( &ptr );
    test( &ptr );
    test( &ptr );
  }
  
  
}
