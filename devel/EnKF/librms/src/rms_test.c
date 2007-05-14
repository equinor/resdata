#include <stdlib.h>
#include <stdio.h>
#include <rms_file.h>
#include <rms_tagkey.h>
#include <rms_stats.h>
#include <list.h>


int main (int argc , char **argv) {
  rms_tagkey_type *mean , *std;
  rms_tag_type    *dim_tag;

  rms_file_type *file = rms_file_alloc("Stats.ROFF" , false);
  rms_file_fread(file);
  rms_file_printf(file , stdout);
  exit(1);
  
  dim_tag = rms_file_fread_alloc_tag(file , "dimensions" , NULL , NULL);
  mean = rms_file_fread_alloc_data_tagkey(file , "parameter" , "name" , "PERMX");
  std  = rms_tagkey_copyc(mean); 
  rms_file_free_data(file);
  rms_stats_mean_std(mean , std , "PERMX" , argc - 1 , (const char **) &argv[1]);
  
  
  
  rms_file_set_filename(file , "Stats.ROFF" , true);

  {
    FILE *stream = rms_file_fopen(file , false);
    rms_file_init_fwrite(file , "parameter" , stream);
    rms_tag_fwrite(dim_tag , stream);
    rms_tag_fwrite_parameter("mean:PERMX" , mean , stream);
    rms_tag_fwrite_parameter("std:PERMX"  , std  , stream);
    rms_file_complete_fwrite(file , stream);
    fclose(stream);
  }
  
  rms_tag_free(dim_tag);
  rms_tagkey_free(mean);
  rms_tagkey_free(std);
  rms_file_free_data(file);  
  rms_file_free(file);

  return 0;
}

