#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <enkf_types.h>
#include <enkf_state.h>
#include <multflt_config.h>
#include <multflt.h>
#include <enkf_util.h>
#include <math.h>


struct multflt_struct {
  const multflt_config_type *config;
  const enkf_state_type     *enkf_state;
  double                    *data;
};

/*****************************************************************/

multflt_type * multflt_alloc(const enkf_state_type * enkf_state , const multflt_config_type * config) {
  multflt_type * multflt  = malloc(sizeof *multflt);
  multflt->enkf_state     = enkf_state;
  multflt->config = config;
  multflt->data           = enkf_util_malloc(config->size * sizeof *multflt->data , __func__);
  return multflt;
}


void multflt_clear(multflt_type * multflt) {
  const int size = multflt_config_get_size(multflt->config);   
  int k;
  for (k = 0; k < size; k++)
    multflt->data[k] = 0.0;
}



multflt_type * multflt_copyc(const multflt_type *multflt) {
  const int size = multflt_config_get_size(multflt->config);   
  multflt_type * new = multflt_alloc(multflt->enkf_state , multflt->config);
  
  memcpy(new->data , multflt->data , size * sizeof *multflt->data);
  return new;
}


char * multflt_alloc_ensname(const multflt_type *multflt) {
  char *ens_name  = enkf_state_alloc_ensname(multflt->enkf_state , multflt_config_get_ensname_ref(multflt->config));
  return ens_name;
}


char * multflt_alloc_eclname(const multflt_type *multflt) {
  char  *ecl_name = enkf_state_alloc_eclname(multflt->enkf_state , multflt_config_get_eclname_ref(multflt->config));
  return ecl_name;
}


void multflt_ecl_write(const multflt_type * multflt) {
  char * ecl_file = multflt_alloc_eclname(multflt);
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const multflt_config_type *config = multflt->config;
    const int size        = config->size;
    const char **fault_names = (const char **) config->fault_names;
    int k;
    fprintf(stream , "MULTFLT\n");
    for (k=0; k < size; k++)
      fprintf(stream , " \'%s\'      %g  / \n",fault_names[k] , multflt->data[k]);
    fprintf(stream , "/");
  }
  
  fclose(stream);
  free(ecl_file);
}


void multflt_ens_write(const multflt_type * multflt) {
  const  multflt_config_type * config = multflt->config;
  char * ens_file = multflt_alloc_ensname(multflt);  
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->size  , sizeof  config->size     , 1 , stream);
  enkf_util_fwrite(multflt->data    , sizeof *multflt->data    , config->size , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void multflt_ens_read(multflt_type * multflt) {
  char * ens_file = multflt_alloc_ensname(multflt);
  FILE * stream   = enkf_util_fopen_r(ens_file , __func__);
  int  nz;
  fread(&nz , sizeof  nz     , 1 , stream);
  enkf_util_fread(multflt->data , sizeof *multflt->data , nz , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void multflt_sample(multflt_type *multflt) {
  const multflt_config_type *config = multflt->config;
  const bool              *active   = config->active;
  const double            *std      = config->std;
  const double            *mean     = config->mean;
  const int                size  = config->size;
  int i;
  
  for (i=0; i < size; i++) 
    if (active[i])
      multflt->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
  
}


void multflt_free(multflt_type *multflt) {
  free(multflt->data);
  free(multflt);
}


void multflt_serialize(const multflt_type *multflt , double *serial_data , size_t *_offset) {
  const multflt_config_type *config = multflt->config;
  const bool              *active   = config->active;
  const int                size  = config->size;
  int offset = *_offset;
  int i;
  
  for (i=0; i < size; i++) 
    if (active[i]) {
      serial_data[offset] = multflt->data[i];
      offset++;
    }
  
  *_offset = offset;
}



MATH_OPS(multflt);

/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC_CONST(multflt_ecl_write , multflt_type)
VOID_FUNC_CONST(multflt_ens_write , multflt_type)
VOID_FUNC      (multflt_ens_read  , multflt_type)
VOID_FUNC      (multflt_sample    , multflt_type)
VOID_FUNC      (multflt_free      , multflt_type)
VOID_SERIALIZE (multflt_serialize , multflt_type)
