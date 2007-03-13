#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rms_roff.h>


static char * alloc_cstring(const char *fort_string , const int *strlen) {
  const char null_char = '\0';
  char *new_string = malloc(*strlen + 1);
  strncpy(new_string , fort_string , *strlen);
  new_string[*strlen] = null_char;
  return new_string;
}


void old_rms_inter_roff_param__(const char *__filename , const int *strlen, const char *__param_name , const int * param_len, float *param) {
  char *filename   = alloc_cstring(__filename   , strlen);
  char *param_name = alloc_cstring(__param_name , param_len);
  rms_roff_load(filename , param_name , param);
  free(filename);
  free(param_name);
}


void rms_inter_roff_param__(const char *__filename , const int *strlen, const char *__param_name , const int * param_len, 
			    const int *nx , const int *ny , const int *nz , 
			    float *undef_rms , float *undef_out,
			    float *param, float *work) {
  char *filename   = alloc_cstring(__filename   , strlen);
  char *param_name = alloc_cstring(__param_name , param_len);

  rms_roff_load(filename , param_name , work);
  {
    int i;
    for (i = 0; i < (*nx) * (*ny) * (*nz); i++)
      if (work[i] == *undef_rms)
	work[i] = *undef_out;
  }
  rms_roff_write_fortran_data(param , work , sizeof *work , *nx , *ny , *nz);

  free(filename);
  free(param_name);
}
