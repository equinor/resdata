#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <util.h>
#include <enkf_types.h>
#include <multz_config.h>
#include <multz.h>
#include <enkf_util.h>


GET_DATA_SIZE_HEADER(multz);

struct multz_struct {
  const multz_config_type *config;
  double                  *data;
};

/*****************************************************************/
void multz_clear(multz_type * multz) {
  const int size = multz_config_get_serial_size(multz->config);   
  int k;
  for (k = 0; k < size; k++)
    multz->data[k] = 0.0;
}


void multz_realloc_data(multz_type *multz) {
  multz->data = enkf_util_calloc(multz_config_get_serial_size(multz->config) , sizeof *multz->data , __func__);
}


void multz_free_data(multz_type *multz) {
  free(multz->data);
  multz->data = NULL;
}


multz_type * multz_alloc(const multz_config_type * multz_config) {
  multz_type * multz  = malloc(sizeof *multz);
  multz->config = multz_config;
  multz->data = NULL;
  multz_realloc_data(multz);
  return multz;
}



char * multz_alloc_ensfile(const multz_type * multz , const char * path) {
  return util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
}


multz_type * multz_copyc(const multz_type *multz) {
  const int size = multz_config_get_serial_size(multz->config);   
  multz_type * new = multz_alloc(multz->config);
  
  memcpy(new->data , multz->data , size * sizeof *multz->data);
  return new;
}


void multz_fread(multz_type * multz , const char *file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  int  size;
  fread(&size , sizeof  size     , 1 , stream);
  enkf_util_fread(multz->data , sizeof *multz->data , size , stream , __func__);
  fclose(stream);
}


void multz_fwrite(const multz_type * multz , const char *file) {
  const  multz_config_type * config = multz->config;
  const int data_size = multz_config_get_data_size(config);
  FILE * stream   = enkf_util_fopen_w(file , __func__);
  
  fwrite(&data_size       ,   sizeof  data_size     , 1 , stream);
  enkf_util_fwrite(multz->data    ,   sizeof *multz->data    ,data_size , stream , __func__);
  
  fclose(stream);
}



void multz_ecl_write(const multz_type * multz , const char * path) {
  char * eclfile = util_alloc_full_path(path , multz_config_get_eclfile_ref(multz->config));
  FILE * stream  = enkf_util_fopen_w(eclfile , __func__);
  {
    const int data_size = multz_config_get_data_size(multz->config);   
    int k;
    for (k=0; k < data_size; k++)
      multz_config_fprintf_layer(multz->config , k , multz->data[k] , stream);
  }
  fclose(stream);
  free(eclfile);
}



void multz_ens_read(multz_type * multz , const char *path) {
  char * ensfile = util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
  multz_fread(multz , ensfile);
  free(ensfile);
}


void multz_ens_write(const multz_type * multz , const char * path) {
  char * ensfile = util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
  multz_fwrite(multz , ensfile);
  free(ensfile);
}


char * multz_swapout(multz_type * multz , const char * path) {
  char * ensfile = util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
  multz_fwrite(multz , ensfile);
  multz_free_data(multz);
  return ensfile;
}


void multz_swapin(multz_type * multz , const char *file) {
  multz_realloc_data(multz);
  multz_fread(multz  , file);
}






void multz_sample(multz_type *multz) {
  const multz_config_type *config    = multz->config;
  const bool              *active    = config->active;
  const double            *std       = config->std;
  const double            *mean      = config->mean;
  const int                data_size = multz_config_get_data_size(config);
  int i;
  
  for (i=0; i < data_size; i++) 
    if (active[i])
      multz->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
    
}




void multz_free(multz_type *multz) {
  multz_free_data(multz);
  free(multz);
}


void multz_serialize(const multz_type *multz , double *serial_data , size_t *_offset) {
  const multz_config_type *config     = multz->config;
  const bool              *active     = config->active;
  const int                data_size  = multz_config_get_data_size(config);
  int offset = *_offset;
  int i;

  for (i=0; i < data_size; i++) 
    if (active[i]) {
      serial_data[offset] = multz->data[i];
      offset++;
    }
  
  *_offset = offset;
}









MATH_OPS(multz)
VOID_ALLOC(multz)
VOID_FREE(multz)
VOID_FREE_DATA(multz)
VOID_REALLOC_DATA(multz)
VOID_ECL_WRITE (multz)
VOID_ENS_WRITE (multz)
VOID_ENS_READ  (multz)
VOID_COPYC     (multz)
VOID_SWAPIN(multz)
VOID_SWAPOUT(multz)
VOID_SERIALIZE(multz)
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC      (multz_clear        , multz_type)
VOID_FUNC      (multz_sample       , multz_type)


