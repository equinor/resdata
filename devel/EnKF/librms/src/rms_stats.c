#include <stdlib.h>
#include <rms_stats.h>
#include <rms_tagkey.h>
#include <rms_tag.h>
#include <rms_file.h>


void rms_stats_mean_std(rms_tagkey_type * mean , rms_tagkey_type * std , const char *parameter_name , int files , const char **filelist) {
  int filenr;

  rms_tagkey_clear(mean);
  rms_tagkey_clear(std);

  for (filenr = 0; filenr < files; filenr++) {
    rms_file_type *rms_file = rms_file_alloc(filelist[filenr] , false);
    rms_tagkey_type * file_tag = rms_file_fread_alloc_data_tagkey(rms_file, "parameter" , "name" , parameter_name);
    printf("Loading from file: %s \n",filelist[filenr]);

    rms_tagkey_inplace_add(mean , file_tag);
    rms_tagkey_inplace_sqr(file_tag);
    rms_tagkey_inplace_add(std , file_tag);
    rms_tagkey_free(file_tag);
    
    rms_file_free(rms_file);
  }
  {
    double norm = 1.0 / files;
    rms_tagkey_type * tmp_mean;
    rms_tagkey_scale(mean , norm);
    rms_tagkey_scale(std  , norm);

    tmp_mean = rms_tagkey_copyc(mean);
    rms_tagkey_inplace_sqr(tmp_mean);
    rms_tagkey_scale(tmp_mean , -1.0);
    rms_tagkey_inplace_add(std , tmp_mean);
    rms_tagkey_inplace_sqrt(std);
    rms_tagkey_free(tmp_mean);
  }

}
