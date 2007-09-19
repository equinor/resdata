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


#define  DEBUG
#define  TARGET_TYPE MULTFLT
#include "enkf_debug.h"


GET_DATA_SIZE_HEADER(multflt);


struct multflt_struct {
  DEBUG_DECLARE
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
  multflt->data  = enkf_util_realloc(multflt->data , multflt_config_get_data_size(multflt->config) * sizeof *multflt->data , __func__);
}


void multflt_set_data(multflt_type * multflt , const double * data) {
  memcpy(multflt->data , data , multflt_config_get_data_size(multflt->config) * sizeof * data);
}


void multflt_get_data(const multflt_type * multflt , double * data) {
  memcpy(data , multflt->data , multflt_config_get_data_size(multflt->config) * sizeof * data);
}



multflt_type * multflt_alloc(const multflt_config_type * config) {
  multflt_type * multflt  = malloc(sizeof *multflt);
  multflt->config = config;
  multflt->data   = NULL;
  multflt_realloc_data(multflt);
  DEBUG_ASSIGN(multflt)
  return multflt;
}


void multflt_clear(multflt_type * multflt) {
  const int size = multflt_config_get_data_size(multflt->config);   
  int k;
  for (k = 0; k < size; k++)
    multflt->data[k] = 0.0;
}


static char * multflt_alloc_ensfile(const multflt_type * multflt , const char * path) {
  return util_alloc_full_path(path , multflt_config_get_ensfile_ref(multflt->config));
}

multflt_type * multflt_copyc(const multflt_type *multflt) {
  const int size = multflt_config_get_data_size(multflt->config);   
  multflt_type * new = multflt_alloc(multflt->config);
  
  memcpy(new->data , multflt->data , size * sizeof *multflt->data);
  return new;
}


void multflt_ecl_write(const multflt_type * multflt, const char * path) {
  char * ecl_file = util_alloc_full_path(path , multflt_config_get_eclfile_ref(multflt->config));
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const multflt_config_type *config = multflt->config;
    const int data_size      = multflt_config_get_data_size(config);
    const char **fault_names = (const char **) config->fault_names;
    int k;
    fprintf(stream , "MULTFLT\n");
    for (k=0; k < data_size; k++)
      fprintf(stream , " \'%s\'      %g  / \n",fault_names[k] , multflt_config_transform(config , k , multflt->data[k]));
    fprintf(stream , "/");
  }
  
  fclose(stream);
  free(ecl_file);
}


void multflt_fwrite(const multflt_type *multflt , const char *file ) {
  const  multflt_config_type * config = multflt->config;
  const int data_size      = multflt_config_get_data_size(config);
  FILE * stream   = enkf_util_fopen_w(file , __func__);
  fwrite(&data_size  , sizeof  data_size     , 1 , stream);
  enkf_util_fwrite(multflt->data    , sizeof *multflt->data    , data_size , stream , __func__);
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


void multflt_truncate(multflt_type * multflt) {
  DEBUG_ASSERT(multflt)
  {
    const multflt_config_type  *config     = multflt->config;
    const int                    data_size  = multflt_config_get_data_size(config);
    int i;
    
    for (i = 0; i < data_size; i++) 
  	multflt->data[i] = multflt_config_truncate(config , i , multflt->data[i]);
    
  }
}



void  multflt_sample(multflt_type *multflt) {
  const multflt_config_type *config  = multflt->config;
  const bool              *active    = config->active;
  const double            *std       = config->std;
  const double            *mean      = config->mean;
  const int                data_size = multflt_config_get_data_size(config);
  int i;
  
  for (i=0; i < data_size; i++) 
    if (active[i])
      multflt->data[i] = enkf_util_rand_normal(mean[i] , std[i]);
  
}



void multflt_serialize(const multflt_type *multflt , double *serial_data , size_t *_offset) {
  const multflt_config_type *config   = multflt->config;
  const bool              *active     = config->active;
  const int                data_size  = multflt_config_get_data_size(config);
  int offset = *_offset;
  int i;
  
  for (i=0; i < data_size; i++) 
    if (active[i]) {
      serial_data[offset] = multflt->data[i];
      offset++;
    }
  
  *_offset = offset;
}


/*****************************************************************/


void multflt_TEST() {
  const char * config_file = "/tmp/multflt_config.txt";
  FILE * stream = util_fopen(config_file , "w");
  fprintf(stream , "North  0.00  1.00   0  TANH \n");
  fprintf(stream , "East   0.00  0.50   0  TANH \n");
  fprintf(stream , "West   0.00  2.00   0  NULL \n");
  fclose(stream);
  
  {
    const int ens_size = 1000;
    char path[64];
    int iens;
    multflt_config_type  * config      = multflt_config_fscanf_alloc(config_file , "MULTFLT.INC" , NULL);
    multflt_type        ** multflt_ens = malloc(ens_size * sizeof * multflt_ens);
    
    for (iens = 0; iens < ens_size; iens++) {
      multflt_ens[iens] = multflt_alloc(config);
      multflt_sample(multflt_ens[iens]);
      sprintf(path , "/tmp/%04d" , iens + 1);
      util_make_path(path);
      multflt_ecl_write(multflt_ens[iens] , path);
    }
  }
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

