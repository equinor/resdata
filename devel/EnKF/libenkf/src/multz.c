#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <util.h>
#include <enkf_types.h>
#include <multz_config.h>
#include <multz.h>
#include <enkf_util.h>



struct multz_struct {
  const multz_config_type *config;
  double                  *data;
};

/*****************************************************************/

void multz_realloc_data(multz_type *multz) {
  multz->data = enkf_util_malloc(multz_config_get_size(multz->config) * sizeof *multz->data , __func__);
}



multz_type * multz_alloc(const multz_config_type * multz_config) {
  multz_type * multz  = malloc(sizeof *multz);
  multz->config = multz_config;
  multz->data = NULL;
  multz_realloc_data(multz);
  return multz;
}




void multz_clear(multz_type * multz) {
  const int size = multz_config_get_size(multz->config);   
  int k;
  for (k = 0; k < size; k++)
    multz->data[k] = 0.0;
}



multz_type * multz_copyc(const multz_type *multz) {
  const int size = multz_config_get_size(multz->config);   
  multz_type * new = multz_alloc(multz->config);
  
  memcpy(new->data , multz->data , size * sizeof *multz->data);
  printf("Har klonet et multz objekt \n");
  return new;
}




void multz_ecl_write(const multz_type * multz , const char * path) {
  char * ecl_file = util_alloc_full_path(path , multz_config_get_ecl_file_ref(multz->config));
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const int size = multz_config_get_size(multz->config);   
    int k;
    for (k=0; k < size; k++)
      multz_config_fprintf_layer(multz->config , k , multz->data[k] , stream);
  }
  fclose(stream);
  free(ecl_file);
}



void multz_ens_write(const multz_type * multz , const char * path) {
  const  multz_config_type * config = multz->config;
  char * ens_file = util_alloc_full_path(path , multz_config_get_ens_file_ref(multz->config));
  
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->size    , sizeof  config->size     , 1 , stream);
  enkf_util_fwrite(multz->data    , sizeof *multz->data    , config->size , stream , __func__);
  fclose(stream);
  free(ens_file);
}


void multz_ens_read(multz_type * multz , const char *path) {
  char * ens_file = util_alloc_full_path(path , multz_config_get_ens_file_ref(multz->config));
  FILE * stream   = enkf_util_fopen_r(ens_file , __func__);
  int  size;
  fread(&size , sizeof  size     , 1 , stream);
  enkf_util_fread(multz->data , sizeof *multz->data , size , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void multz_sample(multz_type *multz) {
  const multz_config_type *config = multz->config;
  const bool              *active = config->active;
  const double            *std    = config->std;
  const double            *mean   = config->mean;
  const int                size     = multz_config_get_size(config);
  int i;

  
  for (i=0; i < size; i++) 
    if (active[i])
      multz->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
    
}


void multz_free_data(multz_type *multz) {
  free(multz->data);
  multz->data = NULL;
}


void multz_free(multz_type *multz) {
  multz_free_data(multz);
  free(multz);
}


void multz_serialize(const multz_type *multz , double *serial_data , size_t *_offset) {
  const multz_config_type *config   = multz->config;
  const bool              *active   = config->active;
  const int                size     = config->size;
  int offset = *_offset;
  int i;
  
  for (i=0; i < size; i++) 
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
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC      (multz_clear        , multz_type)
VOID_FUNC      (multz_sample       , multz_type)
VOID_SERIALIZE (multz_serialize    , multz_type)

