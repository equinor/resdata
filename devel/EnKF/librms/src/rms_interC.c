#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <rms_file.h>
#include <rms_util.h>


void old_rms_roff_load(const char *filename , const char *param_name , float *param);

/*****************************************************************/



void old_rms_inter_roff_param__(const char *__filename , const int *strlen, const char *__param_name , const int * param_len, float *param) {
  char *filename   = util_alloc_cstring(__filename   , strlen);
  char *param_name = util_alloc_cstring(__param_name , param_len);
  old_rms_roff_load(filename , param_name , param);
  free(filename);
  free(param_name);
}


void rms_inter_roff_param__(const char *__filename , const int *strlen, const char *__param_name , const int * param_len, 
			    const int *nx , const int *ny , const int *nz , 
			    float *undef_rms , float *undef_out,
			    float *param, float *work) {
  char *filename   = util_alloc_cstring(__filename   , strlen);
  char *param_name = util_alloc_cstring(__param_name , param_len);

  old_rms_roff_load(filename , param_name , work);
  {
    int i;
    for (i = 0; i < (*nx) * (*ny) * (*nz); i++)
      if (work[i] == *undef_rms)
	work[i] = *undef_out;
  }
  rms_set_fortran_data(param , work , sizeof *work , *nx , *ny , *nz);

  free(filename);
  free(param_name);
}



void rms_inter_roff_param2__(const char *__filename , const int *strlen, const char *__param_name , const int * param_len, 
			     const int *nx , const int *ny , const int *nz , 
			     float *undef_rms , float *undef_out,
			     float *param, float *work) {
  char *filename   = util_alloc_cstring(__filename   , strlen);
  char *param_name = util_alloc_cstring(__param_name , param_len);

  rms_file_type *rms_file = rms_open(filename , false , false);
  rms_file_load(rms_file);
  rms_file_assert_dimensions(rms_file , *nx , *ny , *nz);
  
  {
    rms_tag_type *tag       = rms_file_get_tag(rms_file , "parameter" , "name" , param_name);
    rms_tagkey_type *tagkey = rms_tag_get_key(tag , "data");
    work                    = rms_tagkey_get_data_ref(tagkey);
  }
  
  {
    int i;
    for (i = 0; i < (*nx) * (*ny) * (*nz); i++)
      if (work[i] == *undef_rms)
	work[i] = *undef_out;
  }
  rms_set_fortran_data(param , work , sizeof *work , *nx , *ny , *nz);
  rms_close(rms_file);
  
  free(filename);
  free(param_name);
}
