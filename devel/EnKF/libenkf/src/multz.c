#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <enkf_types.h>
#include <enkf_state.h>
#include <multz_config.h>
#include <multz.h>
#include <enkf_util.h>



struct multz_struct {
  const multz_config_type *config;
  const enkf_state_type   *enkf_state;
  double                  *data;
};

/*****************************************************************/

multz_type * multz_alloc(const enkf_state_type * enkf_state , const multz_config_type * multz_config) {
  multz_type * multz = malloc(sizeof *multz);
  multz->enkf_state   = enkf_state;
  multz->config = multz_config;
  multz->data         = enkf_util_malloc(multz_config_get_size(multz_config) * sizeof *multz->data , __func__);
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
  multz_type * new = multz_alloc(multz->enkf_state , multz->config);
  
  memcpy(new->data , multz->data , size * sizeof *multz->data);
  return new;
}


char * multz_alloc_ensname(const multz_type *multz) {
  char *ens_name  = enkf_state_alloc_ensname(multz->enkf_state , multz_config_get_ensname_ref(multz->config));
  return ens_name;
}


char * multz_alloc_eclname(const multz_type *multz) {
  char  *ecl_name = enkf_state_alloc_eclname(multz->enkf_state , multz_config_get_eclname_ref(multz->config));
  return ecl_name;
}


void multz_ecl_write(const multz_type * multz) {
  char * ecl_file = multz_alloc_eclname(multz);
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const int size = multz_config_get_size(multz->config);   
    int k;
    for (k=0; k < size; k++)
      multz_config_fprintf_layer(multz->config , k + 1 , multz->data[k] , stream);
  }
  fclose(stream);
  free(ecl_file);
}


void multz_ens_write(const multz_type * multz) {
  const  multz_config_type * config = multz->config;
  char * ens_file = multz_alloc_ensname(multz);  
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->size    , sizeof  config->size     , 1 , stream);
  enkf_util_fwrite(multz->data    , sizeof *multz->data    , config->size , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void multz_ens_read(multz_type * multz) {
  char * ens_file = multz_alloc_ensname(multz);
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


void multz_free(multz_type *multz) {
  free(multz->data);
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
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/
VOID_FUNC      (multz_free      , multz_type)
VOID_FUNC_CONST(multz_ecl_write , multz_type)
VOID_FUNC_CONST(multz_ens_write , multz_type)
VOID_FUNC_CONST(multz_copyc     , multz_type)
VOID_FUNC      (multz_ens_read  , multz_type)
VOID_FUNC      (multz_clear     , multz_type)
VOID_FUNC      (multz_sample    , multz_type)
VOID_SERIALIZE (multz_serialize , multz_type)

