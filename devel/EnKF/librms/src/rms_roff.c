#include <stdlib.h>
#include <stdio.h>
#include <rms_roff.h>
#include <string.h>


void rms_roff_load(const char *filename , const char *param_name , float *param) {
  const int offset = 327 + strlen(param_name);
  int n_read;
  int size;
  FILE *stream     = fopen(filename , "r");
  
  fseek(stream , offset , SEEK_SET);
  fread(&size  , 1 , sizeof size , stream);
  n_read = fread(param , sizeof *param , size , stream);
  
  fclose(stream);
  if (n_read != size) {
    fprintf(stderr,"%s: wanted:%d elements - only read:%d - aborting \n",__func__, size , n_read);
    abort();
  }
}



				  
