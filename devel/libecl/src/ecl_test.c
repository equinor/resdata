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
  
  int ministep1 , ministep2 , ministep;
  ecl_sum_data_get_ministep_range( data , &ministep1 , &ministep2);
  printf(" Ministep:%d %d \n",ministep1 , ministep2);
  for (ministep = ministep1; ministep <= ministep2; ministep++) {
    if (ecl_sum_data_has_ministep(data , ministep)) 
      printf("%d  %d  %g  %g \n",ministep , ecl_sum_data_get_sim_time( data , ministep ), ecl_sum_data_get_sim_days( data , ministep ) , ecl_sum_data_get( data , ministep , 442));
  }

  {
    int report_step;
    for (report_step = 0; report_step < 63; report_step++) {
      int m1,m2;
      printf("%d  %d \n",report_step , ecl_sum_data_has_report_step( data , report_step));
      ecl_sum_data_report2ministep_range(data , report_step , &m1 , &m2);
      printf("%d -> [%d,%d] %g\n" , report_step , m1 , m2);
    }
    
    ecl_sum_data_free( data );
    ecl_smspec_free( smspec );
  }
}
