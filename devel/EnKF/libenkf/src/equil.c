#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <enkf_types.h>
#include <enkf_state.h>
#include <equil_config.h>
#include <equil.h>
#include <enkf_util.h>


struct equil_struct {
  const equil_config_type *config;
  const enkf_state_type   *enkf_state;
  double                  *data;
};

/*****************************************************************/

equil_type * equil_alloc(const enkf_state_type * enkf_state , const equil_config_type * config) {
  equil_type * equil    = malloc(sizeof *equil);
  equil->enkf_state     = enkf_state;
  equil->config   = config;
  equil->data           = enkf_util_malloc(config->size * sizeof *equil->data , __func__);
  return equil;
}



void equil_ecl_write(const equil_type * equil) {
  const char * ecl_file = equil_config_get_ecl_file_ref(equil->config);
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const int size = equil->config->size;
    int k;
    for (k=0; k < size; k++)
      /*config_fprintf_layer(equil->config , k + 1 , equil->data[k] , stream);*/
      fprintf(stream , "FAULT:%3d ... %g \n",k,equil->data[k]);
  }
  
  fclose(stream);
}


void equil_ens_write(const equil_type * equil) {
  const  equil_config_type * config = equil->config;
  const char * ens_file = equil_config_get_ens_file_ref(equil->config);
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->size  , sizeof  config->size     , 1 , stream);
  enkf_util_fwrite(equil->data    , sizeof *equil->data    , config->size , stream , __func__);
  fclose(stream);
}



void equil_ens_read(equil_type * equil) {
  const char * ens_file = equil_config_get_ens_file_ref(equil->config);
  FILE * stream   = enkf_util_fopen_r(ens_file , __func__);
  int  nz;
  fread(&nz , sizeof  nz     , 1 , stream);
  enkf_util_fread(equil->data , sizeof *equil->data , nz , stream , __func__);
  fclose(stream);
}



void equil_sample(equil_type *equil) {
  const equil_config_type *config   = equil->config;
  const bool              *active   = config->active;
  const double            *std      = config->std;
  const double            *mean     = config->mean;
  const int                size   = config->size;
  int i;
  
  for (i=0; i < size; i++) 
    if (active[i])
      equil->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
  
}


void equil_free(equil_type *equil) {
  free(equil->data);
  free(equil);
}


MATH_OPS(equil);

/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC_CONST(equil_ecl_write , equil_type)
VOID_FUNC_CONST(equil_ens_write , equil_type)
VOID_FUNC      (equil_ens_read  , equil_type)
VOID_FUNC      (equil_sample    , equil_type)
VOID_FUNC      (equil_free      , equil_type)
