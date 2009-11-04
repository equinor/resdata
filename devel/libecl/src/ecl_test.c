#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_file.h>
#include <ecl_grid.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>
#include <ecl_file.h>
#include <time.h>

int main(int argc , char ** argv) {
  int num_t = 10000;
  ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_case("Gurbat/EXAMPLE_01_BASE");
  time_t start_time = ecl_sum_get_start_time( ecl_sum );
  time_t end_time   = ecl_sum_get_end_time( ecl_sum ); 
  time_t dt         = (end_time - start_time) / (num_t  - 1);
  int i;
  for (i=0; i < num_t; i++) {
    time_t sim_time = start_time + i*dt;
    

    printf("%d  %g  %g \n",sim_time , ecl_sum_get_general_var_from_sim_time( ecl_sum , sim_time , "RPR:2" ),ecl_sum_get_general_var_from_sim_time( ecl_sum , sim_time , "WOPR:OP_1"));
  }
}
