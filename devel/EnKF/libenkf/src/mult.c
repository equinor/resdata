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
}


void mult_get_data(const mult_type * mult , double * data) {
  memcpy(data , mult->data , mult_config_get_data_size(mult->config) * sizeof * data);
}


void mult_realloc_data(mult_type *mult) {
  mult->data = enkf_util_calloc(mult_config_get_data_size(mult->config) , sizeof *mult->data , __func__);
}


void mult_free_data(mult_type *mult) {
  free(mult->data);
  mult->data = NULL;
}


mult_type * mult_alloc(const mult_config_type * mult_config) {
  mult_type * mult  = malloc(sizeof *mult);
  mult->config = mult_config;
  mult->data = NULL;
  mult_realloc_data(mult);
  return mult;
}



char * mult_alloc_ensfile(const mult_type * mult , const char * path) {
  return util_alloc_full_path(path , mult_config_get_ensfile_ref(mult->config));
}


mult_type * mult_copyc(const mult_type *mult) {
  const int size = mult_config_get_serial_size(mult->config);   
  mult_type * new = mult_alloc(mult->config);
  
  memcpy(new->data , mult->data , size * sizeof *mult->data);
  return new;
}


void mult_fread(mult_type * mult , const char *file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  int  size;
  fread(&size , sizeof  size     , 1 , stream);
  enkf_util_fread(mult->data , sizeof *mult->data , size , stream , __func__);
  fclose(stream);
}


void mult_fwrite(const mult_type * mult , const char *file) {
  const  mult_config_type * config = mult->config;
  const int data_size = mult_config_get_data_size(config);
  FILE * stream   = enkf_util_fopen_w(file , __func__);
  
  fwrite(&data_size              ,   sizeof  data_size     , 1 , stream);
  enkf_util_fwrite(mult->data    ,   sizeof *mult->data    ,data_size , stream , __func__);
  
  fclose(stream);
}



void mult_ens_read(mult_type * mult , const char *path) {
  char * ensfile = util_alloc_full_path(path , mult_config_get_ensfile_ref(mult->config));
  mult_fread(mult , ensfile);
  free(ensfile);
}


void mult_ens_write(const mult_type * mult , const char * path) {
  char * ensfile = util_alloc_full_path(path , mult_config_get_ensfile_ref(mult->config));
  mult_fwrite(mult , ensfile);
  free(ensfile);
}



char * mult_swapout(mult_type * mult , const char * path) {
  char * ensfile = util_alloc_full_path(path , mult_config_get_ensfile_ref(mult->config));
  mult_fwrite(mult , ensfile);
  mult_free_data(mult);
  return ensfile;
}



void mult_swapin(mult_type * mult , const char *file) {
  mult_realloc_data(mult);
  mult_fread(mult  , file);
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
}




void mult_free(mult_type *mult) {
  mult_free_data(mult);
  free(mult);
}


void mult_serialize(const mult_type *mult , double *serial_data , size_t *_offset) {
  const mult_config_type *config      = mult->config;
  const bool              *active     = config->active;
  const int                data_size  = mult_config_get_data_size(config);
  int offset = *_offset;
  int i;
  
  for (i=0; i < data_size; i++) 
    if (active[i]) {
      serial_data[offset] = mult->data[i];
      offset++;
    }
  *_offset = offset;
}



void mult_truncate(mult_type * mult) {
  mult_config_truncate(mult->config , mult->data);
}


void mult_transform(mult_type * mult) {
  mult_config_transform(mult->config , mult->data , mult->output_data);
}


/*****************************************************************/


void mult_TEST() {
  return ;
}









MATH_OPS(mult)
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
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC      (mult_clear        , mult_type)
VOID_FUNC      (mult_sample       , mult_type)


