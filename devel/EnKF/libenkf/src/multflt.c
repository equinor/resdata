#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <enkf_types.h>
#include <enkf_state.h>
#include <util.h>
#include <multflt_config.h>
#include <multflt.h>
#include <enkf_util.h>
#include <math.h>


struct multflt_struct {
  const multflt_config_type *config;
  double                    *data;
};

/*****************************************************************/

void multflt_free_data(multflt_type *multflt) {
  free(multflt->data);
  multflt->data = NULL;
}

void multflt_free(multflt_type *multflt) {
  multflt_free_data(multflt);
  free(multflt);
}



void multflt_realloc_data(multflt_type *multflt) {
  multflt->data  = enkf_util_realloc(multflt->data , multflt_config_get_size(multflt->config) * sizeof *multflt->data , __func__);
}


multflt_type * multflt_alloc(const multflt_config_type * config) {
  multflt_type * multflt  = malloc(sizeof *multflt);
  multflt->config = config;
  multflt->data   = NULL;
  multflt_realloc_data(multflt);
  return multflt;
}


void multflt_clear(multflt_type * multflt) {
  const int size = multflt_config_get_size(multflt->config);   
  int k;
  for (k = 0; k < size; k++)
    multflt->data[k] = 0.0;
}


static char * multflt_alloc_ensfile(const multflt_type * multflt , const char * path) {
  return util_alloc_full_path(path , multflt_config_get_ensfile_ref(multflt->config));
}

multflt_type * multflt_copyc(const multflt_type *multflt) {
  const int size = multflt_config_get_size(multflt->config);   
  multflt_type * new = multflt_alloc(multflt->config);
  
  memcpy(new->data , multflt->data , size * sizeof *multflt->data);
  return new;
}


void multflt_ecl_write(const multflt_type * multflt, const char * path) {
  char * ecl_file = util_alloc_full_path(path , multflt_config_get_eclfile_ref(multflt->config));
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


void multflt_fwrite(const multflt_type *multflt , const char *file ) {
  const  multflt_config_type * config = multflt->config;
  FILE * stream   = enkf_util_fopen_w(file , __func__);
  fwrite(&config->size  , sizeof  config->size     , 1 , stream);
  enkf_util_fwrite(multflt->data    , sizeof *multflt->data    , config->size , stream , __func__);
  fclose(stream);
}

void multflt_ens_write(const multflt_type * multflt, const char * path) {
  char * ens_file = multflt_alloc_ensfile(multflt , path);
  multflt_fwrite(multflt,ens_file);
  free(ens_file);
}

void multflt_fread(multflt_type * multflt , const char * file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  int  nz;
  fread(&nz , sizeof  nz     , 1 , stream);
  enkf_util_fread(multflt->data , sizeof *multflt->data , nz , stream , __func__);
  fclose(stream);
}


void multflt_ens_read(multflt_type * multflt , const char * path) {
  char * ens_file = multflt_alloc_ensfile(multflt , path);
  multflt_fread(multflt , ens_file);
  free(ens_file);
}


char * multflt_swapout(multflt_type * multflt , const char * path) {
  char * ensfile = multflt_alloc_ensfile(multflt , path);
  multflt_fwrite(multflt , ensfile);
  multflt_free_data(multflt);
  return ensfile;
}

void multflt_swapin(multflt_type * multflt , const char * file) {
  multflt_realloc_data(multflt);
  multflt_fread(multflt , file);
}


void  multflt_sample(multflt_type *multflt) {
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
VOID_ALLOC(multflt);
VOID_SWAPOUT(multflt);
VOID_SWAPIN(multflt);
VOID_SERIALIZE (multflt)
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_ECL_WRITE (multflt)
VOID_ENS_WRITE (multflt)
VOID_ENS_READ  (multflt)
VOID_COPYC     (multflt)
VOID_FUNC      (multflt_sample    , multflt_type)
VOID_FUNC      (multflt_free      , multflt_type)

