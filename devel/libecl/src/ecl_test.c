#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_vector.h>
#include <ecl_grid.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>

int main(int argc, char ** argv) {
  ecl_smspec_type   * smspec = ecl_smspec_fread_alloc("Gurbat/EXAMPLE_01_BASE.SMSPEC" , true);
  ecl_sum_data_type * data   = ecl_sum_data_fread_alloc(smspec , argc - 1 , (const char **) &argv[1] , true);  
  
  {
    int i;
    for (i=0; i < 500; i++)
      printf("%03d: %d \n",i,ecl_sum_data_has_ministep( data , i ));
  }
  
  ecl_sum_data_free( data );
  ecl_smspec_free( smspec );
}
