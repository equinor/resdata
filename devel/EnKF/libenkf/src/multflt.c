#include <stdlib.h>
#include <stdio.h>
#include <enkf_types.h>
#include <multflt_config.h>
#include <multflt.h>
#include <enkf_util.h>


struct multflt_struct {
  const multflt_config_type *multflt_config;
  const mem_config_type   *mem_config;
  double                  *data;
};

/*****************************************************************/

multflt_type * multflt_alloc(const mem_config_type * mem_config , const multflt_config_type * multflt_config) {
  multflt_type * multflt = malloc(sizeof *multflt);
  multflt->mem_config     = mem_config;
  multflt->multflt_config = multflt_config;
  multflt->data           = enkf_util_malloc(multflt_config->nfaults * sizeof *multflt->data , __func__);
  return multflt;
}



char * multflt_alloc_ensname(const multflt_type *multflt) {
  char *ens_name  = mem_config_alloc_ensname(multflt->mem_config , multflt_config_get_ensname_ref(multflt->multflt_config));
  return ens_name;
}


char * multflt_alloc_eclname(const multflt_type *multflt) {
  char  *ecl_name = mem_config_alloc_eclname(multflt->mem_config , multflt_config_get_eclname_ref(multflt->multflt_config));
  return ecl_name;
}


void multflt_ecl_write(const multflt_type * multflt) {
  char * ecl_file = multflt_alloc_eclname(multflt);
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const int nfaults = multflt->multflt_config->nfaults;
    int k;
    for (k=0; k < nfaults; k++)
      /*multflt_config_fprintf_layer(multflt->multflt_config , k + 1 , multflt->data[k] , stream);*/
      fprintf(stream , "FAULT ... %g \n",multflt->data[k]);
  }
  
  fclose(stream);
  free(ecl_file);
}


void multflt_ens_write(const multflt_type * multflt) {
  const  multflt_config_type * config = multflt->multflt_config;
  char * ens_file = multflt_alloc_ensname(multflt);  
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->nfaults  , sizeof  config->nfaults     , 1 , stream);
  enkf_util_fwrite(multflt->data    , sizeof *multflt->data    , config->nfaults , stream , __func__);
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
  const multflt_config_type *config = multflt->multflt_config;
  const bool              *active   = config->active;
  const double            *std      = config->std;
  const double            *mean     = config->mean;
  const int                nfaults  = config->nfaults;
  int i;
  
  for (i=0; i < nfaults; i++) 
    if (active[i])
      multflt->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
  
}


void multflt_free(multflt_type *multflt) {
  free(multflt->data);
  free(multflt);
}


/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC_CONST(multflt_ecl_write , multflt_type)
VOID_FUNC_CONST(multflt_ens_write , multflt_type)
VOID_FUNC      (multflt_ens_read  , multflt_type)
VOID_FUNC      (multflt_sample    , multflt_type)
VOID_FUNC      (multflt_free      , multflt_type)
