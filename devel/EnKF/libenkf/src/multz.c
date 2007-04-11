#include <stdlib.h>
#include <stdio.h>
#include <enkf_types.h>
#include <multz_config.h>
#include <multz.h>
#include <enkf_util.h>


struct multz_struct {
  const multz_config_type *multz_config;
  const mem_config_type   *mem_config;
  double                  *data;
};

/*****************************************************************/

multz_type * multz_alloc(const mem_config_type * mem_config , const multz_config_type * multz_config) {
  multz_type * multz = malloc(sizeof *multz);
  multz->mem_config   = mem_config;
  multz->multz_config = multz_config;
  multz->data         = enkf_util_malloc(multz_config_get_nz(multz_config) * sizeof *multz->data , __func__);
  return multz;
}



char * multz_alloc_ensname(const multz_type *multz) {
  char *ens_name  = mem_config_alloc_ensname(multz->mem_config , multz_config_get_ensname_ref(multz->multz_config));
  return ens_name;
}


char * multz_alloc_eclname(const multz_type *multz) {
  char  *ecl_name = mem_config_alloc_eclname(multz->mem_config , multz_config_get_eclname_ref(multz->multz_config));
  return ecl_name;
}


void multz_ecl_write(const multz_type * multz) {
  char * ecl_file = multz_alloc_eclname(multz);
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const int nz = multz_config_get_nz(multz->multz_config);
    int k;
    for (k=0; k < nz; k++)
      multz_config_fprintf_layer(multz->multz_config , k + 1 , multz->data[k] , stream);
  }
  
  fclose(stream);
  free(ecl_file);
}


void multz_ens_write(const multz_type * multz) {
  const  multz_config_type * config = multz->multz_config;
  char * ens_file = multz_alloc_ensname(multz);  
  FILE * stream   = enkf_util_fopen_w(ens_file , __func__);
  fwrite(&config->nz    , sizeof  config->nz     , 1 , stream);
  enkf_util_fwrite(multz->data    , sizeof *multz->data    , config->nz , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void multz_ens_read(multz_type * multz) {
  char * ens_file = multz_alloc_ensname(multz);
  FILE * stream   = enkf_util_fopen_r(ens_file , __func__);
  int  nz;
  fread(&nz , sizeof  nz     , 1 , stream);
  enkf_util_fread(multz->data , sizeof *multz->data , nz , stream , __func__);
  fclose(stream);
  free(ens_file);
}



void multz_sample(multz_type *multz) {
  const multz_config_type *config = multz->multz_config;
  const bool              *active = config->active;
  const double            *std    = config->std;
  const double            *mean   = config->mean;
  const int                nz     = multz_config_get_nz(config);
  int i;
  
  for (i=0; i < nz; i++) 
    if (active[i])
      multz->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
    
}


void multz_free(multz_type *multz) {
  free(multz->data);
  free(multz);
}


/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC_CONST(multz_ecl_write , multz_type)
VOID_FUNC_CONST(multz_ens_write , multz_type)
VOID_FUNC      (multz_ens_read  , multz_type)
VOID_FUNC      (multz_sample    , multz_type)
