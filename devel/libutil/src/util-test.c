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
#include <conf.h>


typedef struct {
  double x,y,z;
} point_type;



int main(int argc , char ** argv) {
  vector_type * points = vector_alloc_new();
  FILE * stream        = util_fopen("file" , "r");
  {
    point_type tmp_point;
    if (fscanf(stream , "%g %g %g" , &tmp_point.x , &tmp_point.y , &tmp_point.z) == 3) {
      
    }
  }

}
