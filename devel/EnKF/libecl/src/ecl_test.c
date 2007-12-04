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


int main(int argc, char ** argv) {
  int files;
  const char *path    = "/h/a152128/EnKF_ON/Run-FMT-Test/PriorEns/tmpdir_0001";
  char ** file_list   = ecl_util_alloc_scandir_filelist(path , "ECL-0001" , ecl_summary_file , false , &files);
  char  * header_file = ecl_util_alloc_exfilename(path , "ECL-0001" , ecl_summary_header_file , false , -1);

  {
    ecl_sum_type * ecl_sum = ecl_sum_fread_alloc(header_file , files , (const char **) file_list , true , true);
    ecl_sum_free(ecl_sum);
  }

  util_free_string_list(file_list , files);
  free(header_file);

  ecl_diag_ens_interactive("tmpdir_" , NULL , false , false , true);
}
