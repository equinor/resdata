#include <stdlib.h>
#include <stdio.h>
#include <enkf_types.h>
#include <enkf_state.h>
#include <equil_config.h>
#include <equil.h>
#include <enkf_util.h>


struct equil_struct {
  const equil_config_type *equil_config;
  const enkf_state_type   *enkf_state;
  double                  *data;
};

/*****************************************************************/

equil_type * equil_alloc(const enkf_state_type * enkf_state , const equil_config_type * equil_config) {
  equil_type * equil    = malloc(sizeof *equil);
  equil->enkf_state     = enkf_state;
  equil->equil_config   = equil_config;
  equil->data           = enkf_util_malloc(equil_config->nequil * sizeof *equil->data , __func__);
  return equil;
}



char * equil_alloc_ensname(const equil_type *equil) {
  char *ens_name  = enkf_state_alloc_ensname(equil->enkf_state , equil_config_get_ensname_ref(equil->equil_config));
  return ens_name;
}


char * equil_alloc_eclname(const equil_type *equil) {
  char  *ecl_name = enkf_state_alloc_eclname(equil->enkf_state , equil_config_get_eclname_ref(equil->equil_config));
  return ecl_name;
}


void equil_ecl_write(const equil_type * equil) {
  char * ecl_file = equil_alloc_eclname(equil);
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const int nequil = equil->equil_config->nequil;
    int k;
    for (k=0; k < nequil; k++)
      /*equil_config_fprintf_layer(equil->equil_config , k + 1 , equil->data[k] , stream);*/
      fprintf(stream , "FAULT:%3d ... %g \n",k,equil->data[k]);
  }
  
  fclose(stream);
  free(ecl_file);
}


void equil_ens_write(const equil_type * equil) {
  const  equil_config_type * config = equil->equil_config;
  char * ens_file = equil_alloc_ensname(equil);  
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->nequil  , sizeof  config->nequil     , 1 , stream);
  enkf_util_fwrite(equil->data    , sizeof *equil->data    , config->nequil , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void equil_ens_read(equil_type * equil) {
  char * ens_file = equil_alloc_ensname(equil);
  FILE * stream   = enkf_util_fopen_r(ens_file , __func__);
  int  nz;
  fread(&nz , sizeof  nz     , 1 , stream);
  enkf_util_fread(equil->data , sizeof *equil->data , nz , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void equil_sample(equil_type *equil) {
  const equil_config_type *config   = equil->equil_config;
  const bool              *active   = config->active;
  const double            *std      = config->std;
  const double            *mean     = config->mean;
  const int                nequil   = config->nequil;
  int i;
  
  for (i=0; i < nequil; i++) 
    if (active[i])
      equil->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
  
}


void equil_free(equil_type *equil) {
  free(equil->data);
  free(equil);
}


/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC_CONST(equil_ecl_write , equil_type)
VOID_FUNC_CONST(equil_ens_write , equil_type)
VOID_FUNC      (equil_ens_read  , equil_type)
VOID_FUNC      (equil_sample    , equil_type)
VOID_FUNC      (equil_free      , equil_type)
