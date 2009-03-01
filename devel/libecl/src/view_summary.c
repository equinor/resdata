#include <ecl_kw.h>
#include <stdlib.h>
#include <ecl_sum.h>
#include <util.h>


int main(int argc , char ** argv) {
  if (argc < 3) {
    fprintf(stderr,"** ERROR ** %s ECLIPSE.DATA WOPR:WGNAME \n",__func__);
    exit(1);
  }
  {
    const char * data_file = argv[1];
    ecl_sum_type * ecl_sum;

    ecl_sum = ecl_sum_fread_alloc_case( data_file , true );
    ecl_sum_fprintf(ecl_sum , stdout , argc - 2 , (const char **) &argv[2]);
    ecl_sum_free(ecl_sum);
  }
    

}
