#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>



int main(int argc, char ** argv) {
#define nvar 3
  const double var_opr = 1000  * 1000;
  const double var_gpr = 10000 * 10000;
  const double var_wpr = 100 * 100;

  const char * path = "/h/a152128/EnKF_OS/EnKF/Relperm_run1/PriorExp_a/tmpdir_0015";
  const char * base = "ECLIPSE-0015";
  char ** var_list;
  
  ecl_sum_type * ecl_sum;
  char ** fileList;
  char *  header_file;
  int     files , i;
  double  *inv_covar;
  
  var_list    = malloc(nvar * sizeof * var_list);
  var_list[0] = util_alloc_string_copy("WOPR");
  var_list[1] = util_alloc_string_copy("WWPR");
  var_list[2] = util_alloc_string_copy("WGPR");
  
  inv_covar = malloc(nvar * nvar * sizeof * inv_covar);

  header_file = ecl_util_alloc_exfilename(path , base , ecl_summary_header_file , false , -1);
  fileList = ecl_util_alloc_exfilelist(path , base , ecl_summary_file , false , &files); 
  ecl_sum = ecl_sum_fread_alloc(header_file , files , (const char **) fileList , true , true);
  for (i = 0; i < nvar*nvar; i++)
    inv_covar[i] = 0.0;

  inv_covar[0] = 1.0 / var_opr;
  inv_covar[4] = 1.0 / var_wpr;
  inv_covar[8] = 1.0 / var_gpr;
  
  {
    double misfit = ecl_sum_eval_misfit(ecl_sum , nvar , (const char **) var_list , inv_covar , NULL);
    printf("misfit: %g \n",misfit);
  }

  printf("Summary loaded ...\n");
  
  return 0;
}
