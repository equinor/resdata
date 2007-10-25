#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <util.h>
#include <enkf_types.h>
#include <mult_config.h>
#include <mult.h>
#include <enkf_util.h>
#include <logmode.h>




/*****************************************************************/

GET_DATA_SIZE_HEADER(mult);

struct mult_struct {
  const mult_config_type *config;
  double                  *data;
  double                  *output_data;
  bool                     output_valid;
};

/*****************************************************************/
void mult_clear(mult_type * mult) {
  const int size = mult_config_get_data_size(mult->config);   
  int k;
  for (k = 0; k < size; k++)
    mult->data[k] = 0.0;
}

void mult_set_data(mult_type * mult , const double * data) {
  memcpy(mult->data , data , mult_config_get_data_size(mult->config) * sizeof * data);
  mult->output_valid = false;
}


void mult_get_data(const mult_type * mult , double * data) {
  memcpy(data , mult->data , mult_config_get_data_size(mult->config) * sizeof * data);
}

void mult_get_output_data(const mult_type * mult , double * output_data) {
  memcpy(output_data , mult->output_data , mult_config_get_data_size(mult->config) * sizeof * output_data);
}


void mult_realloc_data(mult_type *mult) {
  mult->data        = enkf_util_calloc(mult_config_get_data_size(mult->config) , sizeof *mult->data        , __func__);
  mult->output_data = enkf_util_calloc(mult_config_get_data_size(mult->config) , sizeof *mult->output_data , __func__);
}


void mult_free_data(mult_type *mult) {
  free(mult->data);
  free(mult->output_data);
  mult->data        = NULL;
  mult->output_data = NULL;
}


mult_type * mult_alloc(const mult_config_type * mult_config) {
  mult_type * mult  = malloc(sizeof *mult);
  mult->config = mult_config;
  mult->data        = NULL;
  mult->output_data = NULL;
  mult_realloc_data(mult);
  return mult;
}


void mult_memcpy(mult_type * new, const mult_type * old) {
  int size = mult_config_get_data_size(old->config);
  memcpy(new->data , old->data , size * sizeof *old->data);
  new->output_valid = false;
}



mult_type * mult_copyc(const mult_type *mult) {
  mult_type * new = mult_alloc(mult->config);
  mult_memcpy(new , mult);
  return new;
}


void mult_stream_fread(mult_type * mult , FILE * stream) {

  int  size;
  fread(&size , sizeof  size     , 1 , stream);
  enkf_util_fread(mult->data , sizeof *mult->data , size , stream , __func__);
  fclose(stream);
  mult->output_valid = false;

}


void mult_stream_fwrite(const mult_type * mult , FILE * stream) {

  const int data_size = mult_config_get_data_size(mult->config);
  fwrite(&data_size              ,   sizeof  data_size     , 1 , stream);
  enkf_util_fwrite(mult->data    ,   sizeof *mult->data    ,data_size , stream , __func__);
  
}


char * mult_swapout(mult_type * mult , const char * path) {
  /*char * ensfile = util_alloc_full_path(path , mult_config_get_ensfile_ref(mult->config));
  mult_fwrite(mult , ensfile);
  mult_free_data(mult);
  return ensfile;
  */
  fprintf(stderr,"%s: not implemented - aborting \n",__func__);
  abort();
}



void mult_swapin(mult_type * mult , const char *file) {
  /*
    mult_realloc_data(mult);
    mult_fread(mult  , file);
  */
  fprintf(stderr,"%s: not implemented - aborting \n",__func__);
  abort();
}




void mult_sample(mult_type *mult) {
  const mult_config_type *config    = mult->config;
  const bool              *active    = config->active;
  const double            *std       = config->std;
  const double            *mean      = config->mean;
  const int                data_size = mult_config_get_data_size(config);
  int i;
  
  for (i=0; i < data_size; i++) 
    if (active[i])
      mult->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
  mult->output_valid = false;
}




void mult_free(mult_type *mult) {
  mult_free_data(mult);
  free(mult);
}


int mult_serialize(const mult_type *mult , size_t serial_data_size ,  double *serial_data , size_t stride , size_t offset) {
  const mult_config_type *config      = mult->config;
  const bool              *active     = config->active;
  const int                data_size  = mult_config_get_data_size(config);
  const int internal_offset = 0;
  int elements_added = 0;
  int i;
  
  for (i=internal_offset; i < data_size; i++) {
    size_t serial_index = offset + i*stride;
    if (serial_index < serial_data_size) {
      if (active[i]) {
	serial_data[offset + i*stride] = mult->data[i];
	elements_added++;
      }
    } else break;
  } 
  return elements_added;
}


void mult_truncate(mult_type * mult) {
  mult_config_truncate(mult->config , mult->data);
}


void mult_transform(mult_type * mult) {
  mult_config_transform(mult->config , mult->data , mult->output_data);
  mult->output_valid = true;
}

const double * mult_get_output_ref(const mult_type * mult) { return mult->output_data; }
const double * mult_get_data_ref  (const mult_type * mult) { return mult->data; }


/*
void mult_iadd(void *void_arg , const void *void_delta) {                
  mult_type *arg         = (mult_type *)       void_arg;  	       
  const mult_type *delta = (const mult_type *) void_delta;	       
  const mult_config_type *config = arg->config; 			       
  const int data_size = config->data_size;                      			       
  int i;                                              			       
  if (config != delta->config) {                                                 
    fprintf(stderr,"%s:two multz object have different config objects - aborting \n",__func__);
    abort();                                                                   
  }                                                                              
  for (i=0; i < data_size; i++) {                           			       
    arg->data[i]        += delta->data[i];
    arg->output_data[i] += delta->output_data[i];
  }
}


void mult_iscale(void *void_arg , double scale_factor) {                
  mult_type *arg                 = (mult_type *)       void_arg;  	       
  const mult_config_type *config = arg->config; 			       
  const int data_size            = config->data_size;                      			       
  int i;                                              			       

  for (i=0; i < data_size; i++) {                           			       
    arg->data[i]        *= scale_factor;
    arg->output_data[i] *= scale_factor;
  }
}
*/




/*****************************************************************/


void mult_TEST() {
  return ;
}




MATH_OPS(mult)

/*
VOID_ALLOC(mult)
VOID_FREE(mult)
VOID_FREE_DATA(mult)
VOID_REALLOC_DATA(mult)
VOID_ENS_WRITE (mult)
VOID_ENS_READ  (mult)
VOID_COPYC     (mult)
VOID_SWAPIN(mult)
VOID_SWAPOUT(mult)
VOID_SERIALIZE(mult)
VOID_TRUNCATE(mult)



VOID_FUNC      (mult_clear        , mult_type)
VOID_FUNC      (mult_sample       , mult_type)
*/

