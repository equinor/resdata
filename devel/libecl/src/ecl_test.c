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
#include <ecl_diag.h>


int main(int argc, char ** argv) {
  int files;
  const char *path    = "/tmp/enkf/testing";
  char ** file_list   = ecl_util_alloc_scandir_filelist(path , "OEAST-0088" , ecl_summary_file , false , &files);
  char  * header_file = ecl_util_alloc_exfilename(path , "OEAST-0088" , ecl_summary_header_file , false , -1);
  
  {
    ecl_sum_type * ecl_sum = ecl_sum_fread_alloc_interactive(true);
    if (ecl_sum != NULL) {
      printf("Well_var: %g \n",ecl_sum_get_well_var(ecl_sum , 400 , "E7A" , "WOPR"));
      printf("Region pressure: %g \n",ecl_sum_get_region_var(ecl_sum , 400 , 0 , "RPR"));
      printf("Group OPT : %g \n",ecl_sum_get_group_var(ecl_sum , 400 , "SOUTH"  , "GOPT"));
      printf("Group OPT : %g \n",ecl_sum_get_group_var(ecl_sum , 400 , "SADDLE" , "GOPT"));
      
      ecl_sum_free(ecl_sum);
    }
  }

  util_free_string_list(file_list , files);
  free(header_file);
}
