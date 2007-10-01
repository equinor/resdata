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
#include <mult.h>


#define  DEBUG
#define  TARGET_TYPE MULTFLT
#include "enkf_debug.h"


GET_DATA_SIZE_HEADER(multflt);


struct multflt_struct {
  DEBUG_DECLARE
  const multflt_config_type *config;
  mult_type                 *mult;
};

/*****************************************************************/

void multflt_free_data(multflt_type *multflt) {
  mult_free(multflt->mult);
}



void multflt_free(multflt_type *multflt) {
  multflt_free_data(multflt);
  free(multflt);
}



void multflt_realloc_data(multflt_type *multflt) {
  mult_realloc_data(multflt->mult);
}


void multflt_output_transform(const multflt_type * multflt) {
  mult_transform(multflt->mult);
}

void multflt_set_data(multflt_type * multflt , const double * data) {
  mult_set_data(multflt->mult , data);
}


void multflt_get_data(const multflt_type * multflt , double * data) {
  mult_get_data(multflt->mult , data);
}

void multflt_get_output_data(const multflt_type * multflt , double * output_data) {
  mult_get_output_data(multflt->mult , output_data);
}


const double * multflt_get_data_ref(const multflt_type * multflt) {
  return mult_get_data_ref(multflt->mult);
}


const double * multflt_get_output_ref(const multflt_type * multflt) {
  return mult_get_output_ref(multflt->mult);
}


multflt_type * multflt_alloc(const multflt_config_type * config) {
  multflt_type * multflt  = malloc(sizeof *multflt);
  multflt->config = config;
  multflt->mult   = mult_alloc(config->mult_config); 
  DEBUG_ASSIGN(multflt)
  return multflt;
}


void multflt_clear(multflt_type * multflt) {
  mult_clear(multflt->mult);
}


static char * multflt_alloc_ensfile(const multflt_type * multflt , const char * path) {
  return util_alloc_full_path(path , multflt_config_get_ensfile_ref(multflt->config));
}


multflt_type * multflt_copyc(const multflt_type *multflt) {
  multflt_type * new = multflt_alloc(multflt->config); 
  mult_memcpy(new->mult , multflt->mult);
  return new; 
}



void multflt_ecl_write(const multflt_type * multflt, const char * path) {
  char * ecl_file = util_alloc_full_path(path , multflt_config_get_eclfile_ref(multflt->config));
  FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
  {
    const multflt_config_type *config = multflt->config;
    const int data_size       = multflt_config_get_data_size(config);
    const char **fault_names  = (const char **) config->fault_names;
    const double *output_data = mult_get_output_ref(multflt->mult);
    int k;
    
    multflt_output_transform(multflt);
    fprintf(stream , "MULTFLT\n");
    for (k=0; k < data_size; k++)
      fprintf(stream , " \'%s\'      %g  / \n",fault_names[k] , output_data[k]);
    fprintf(stream , "/");
  }
  
  fclose(stream);
  free(ecl_file);


  {
    ecl_file = util_alloc_full_path(path , "MULTFLT.GAUSS");
    FILE * stream   = enkf_util_fopen_w(ecl_file , __func__);
    {
      const multflt_config_type *config = multflt->config;
      const int data_size       = multflt_config_get_data_size(config);
      const char **fault_names  = (const char **) config->fault_names;
      const double *data        = mult_get_data_ref(multflt->mult);
      int k;
      
      for (k=0; k < data_size; k++)
	fprintf(stream , " \'%s\'      %g  / \n",fault_names[k] , data[k]);
    }
    
    fclose(stream);
    free(ecl_file);
  }
}





void multflt_fwrite(const multflt_type *multflt , const char *file ) {
  FILE * stream   = enkf_util_fopen_w(file , __func__);
  mult_stream_fwrite(multflt->mult , stream);
  fclose(stream);
}

void multflt_ens_write(const multflt_type * multflt, const char * path) {
  char * ens_file = multflt_alloc_ensfile(multflt , path);
  multflt_fwrite(multflt,ens_file);
  free(ens_file);
}

void multflt_fread(multflt_type * multflt , const char * file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  mult_stream_fread(multflt->mult , stream);
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
  mult_truncate( multflt->mult );  
}



void  multflt_sample(multflt_type *multflt) {
  DEBUG_ASSERT(multflt)
  mult_sample(multflt->mult);  
}



void multflt_serialize(const multflt_type *multflt , double *serial_data , size_t *offset) {
  mult_serialize(multflt->mult , serial_data , offset);
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




/*
  Mathops not implemented ... 
*/

MATH_OPS_MULT(multflt);
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

