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


int main(int argc, char ** argv) {
  ecl_sum_type * ecl_sum;
  int files;
  char ** fileList = ecl_util_alloc_scandir_filelist("/h/a152128/EnKF_ON/Run-FMT-Test/PriorEns/tmpdir_0001" , "ECL-0001" , ecl_summary_file , false , &files);
  ecl_sum = ecl_sum_fread_alloc("/h/a152128/EnKF_ON/Run-FMT-Test/PriorEns/tmpdir_0001/ECL-0001.SMSPEC", files, (const char **) fileList ,  false , true);

  {
    int i;
    for (i=0; i < files; i++) {
      printf("%d  %g \n",i,ecl_sum_iget(ecl_sum , i , "B-43A" , "WGPRH") / ecl_sum_iget(ecl_sum , i , "B-43A" , "WGPR"));
    }
  }

  ecl_sum_free(ecl_sum);
}
