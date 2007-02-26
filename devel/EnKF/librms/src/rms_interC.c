#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rms_load.h>


static char * alloc_cstring(const char *fort_string , const int *strlen) {
  const char null_char = '\0';
  char *new_string = malloc(*strlen + 1);
  strncpy(new_string , fort_string , *strlen);
  new_string[*strlen] = null_char;
  return new_string;
}


void rms_inter_load_param__(const char *__filename , const int *strlen, const char *__param_name , const int * param_len, float *param) {
  char *filename   = alloc_cstring(__filename   , strlen);
  char *param_name = alloc_cstring(__param_name , param_len);
  rms_load(filename , param_name , param);
  free(filename);
  free(param_name);
}
