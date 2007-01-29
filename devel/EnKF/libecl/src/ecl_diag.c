#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <ecl_fstate.h>
#include <util.h>
#include <ecl_sum.h>


ecl_sum_type ** ecl_diag_load_ensemble(int iens1, int iens2 , const char *ens_path , const char *eclbase_dir , const char *eclbase, bool fmt_file, bool unified) {
  ecl_sum_type **ecl_sum_list = calloc(iens2 - iens1 + 1 , sizeof(ecl_sum_type *));
  char spec_file[512];
  int iens;
  int fmt_mode;
  if (fmt_file)
    fmt_mode = ECL_FORMATTED;
  else
    fmt_mode = ECL_BINARY;

  for (iens = iens1; iens <= iens2; iens++) {
    if (fmt_file)
      sprintf(spec_file , "%s/%s%04d/%s-%04d.FSMSPEC" , ens_path , eclbase_dir , iens, eclbase ,iens);
    else
      sprintf(spec_file , "%s/%s%04d/%s-%04d.SMSPEC" , ens_path , eclbase_dir , iens, eclbase ,iens);
    
    if (unified) {
      char data_file[512];
      if (fmt_file)
	sprintf(data_file , "%s/%s%04d/%s-%04d.FUNSMRY" , ens_path , eclbase_dir , iens, eclbase ,iens);
      else
	sprintf(data_file , "%s/%s%04d/%s-%04d.UNSMRY" , ens_path , eclbase_dir , iens, eclbase ,iens);
      printf("Loading file: %s ",data_file); fflush(stdout);
      ecl_sum_list[iens - iens1] = ecl_sum_load_unified(spec_file , data_file , fmt_mode , true);
      printf("\n");
    } else {
      int files;
      char _path[512];
      char _base[512];
      char **fileList;
      sprintf(_path , "%s/%s%04d" , ens_path , eclbase_dir , iens);
      sprintf(_base , "%s-%04d"   , eclbase  , iens);
      fileList  = ecl_sum_alloc_filelist(_path , _base , fmt_file , &files);
      printf("Loading from directory: %s",_path); fflush(stdout);
      ecl_sum_list[iens - iens1] = ecl_sum_load_multiple(spec_file , files , (const char **) fileList , fmt_mode , true);
      printf("\n");
      util_free_string_list(fileList , files);
    }
  }
  return ecl_sum_list;
}

