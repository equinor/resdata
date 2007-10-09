#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <util.h>
#include <enkf_types.h>
#include <multz_config.h>
#include <multz.h>
#include <enkf_util.h>
#include <mult.h>


#define  DEBUG
#define  TARGET_TYPE MULTZ
#include "enkf_debug.h"


/*****************************************************************/

GET_DATA_SIZE_HEADER(multz);

struct multz_struct {
  DEBUG_DECLARE
  const multz_config_type *config;
  mult_type               *mult;
};

/*****************************************************************/
void multz_clear(multz_type * multz) {
  mult_clear(multz->mult);
}




void multz_output_transform(const multz_type * multz) {
  mult_transform(multz->mult);
}

void multz_set_data(multz_type * multz , const double * data) {
  mult_set_data(multz->mult , data);
}


void multz_get_data(const multz_type * multz , double * data) {
  mult_get_data(multz->mult , data);
}

void multz_get_output_data(const multz_type * multz , double * output_data) {
  mult_get_output_data(multz->mult , output_data);
}


const double * multz_get_data_ref(const multz_type * multz) {
  return mult_get_data_ref(multz->mult);
}


const double * multz_get_output_ref(const multz_type * multz) {
  return mult_get_output_ref(multz->mult);
}




void multz_realloc_data(multz_type *multz) {
  mult_realloc_data(multz->mult);
}


void multz_free_data(multz_type *multz) {
  mult_free(multz->mult);
}


multz_type * multz_alloc(const multz_config_type * multz_config) {
  multz_type * multz  = malloc(sizeof *multz);
  multz->config = multz_config;
  multz->mult   = mult_alloc(multz_config->mult_config); 
  DEBUG_ASSIGN(multz)
  return multz;
}



char * multz_alloc_ensfile(const multz_type * multz , const char * path) {
  return util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
}

multz_type * multz_copyc(const multz_type *multz) {
  multz_type * new = multz_alloc(multz->config); 
  mult_memcpy(new->mult , multz->mult);
  return new; 
}


void multz_fwrite(const multz_type *multz , const char *file ) {
  FILE * stream   = enkf_util_fopen_w(file , __func__);
  mult_stream_fwrite(multz->mult , stream);
  fclose(stream);
}


void multz_fread(multz_type * multz , const char * file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  mult_stream_fread(multz->mult , stream);
  fclose(stream);
}



void multz_ecl_write(const multz_type * multz , const char * path) {
  DEBUG_ASSERT(multz) 
  {
    char * eclfile = util_alloc_full_path(path , multz_config_get_eclfile_ref(multz->config));
    FILE * stream  = enkf_util_fopen_w(eclfile , __func__);
    
    multz_output_transform(multz);
    multz_config_ecl_write(multz->config , multz_get_output_ref(multz) , stream);
    
    fclose(stream);
    free(eclfile);
  }
}



/*void multz_direct_ecl_write(const multz_type * multz , const char * path) {
  char * eclfile = util_alloc_full_path(path , multz_config_get_eclfile_ref(multz->config));
  FILE * stream  = enkf_util_fopen_w(eclfile , __func__);

  multz_config_ecl_write(multz->config , multz_get_output_ref(multz) , stream);
  
  fclose(stream);
  free(eclfile);
}
*/



void multz_ens_read(multz_type * multz , const char *path) {
  char * ensfile = util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
  multz_fread(multz , ensfile);
  free(ensfile);
}


void multz_ens_write(const multz_type * multz , const char * path) {
  DEBUG_ASSERT(multz)
  {
     char * ensfile = util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
     multz_fwrite(multz , ensfile);
     free(ensfile);
  }
}



char * multz_swapout(multz_type * multz , const char * path) {
  DEBUG_ASSERT(multz)
  {
    char * ensfile = util_alloc_full_path(path , multz_config_get_ensfile_ref(multz->config));
    multz_fwrite(multz , ensfile);
    multz_free_data(multz);
    return ensfile;
  }
}



void multz_swapin(multz_type * multz , const char *file) {
  DEBUG_ASSERT(multz)
  {
    multz_realloc_data(multz);
    multz_fread(multz  , file);
  }
}






void multz_free(multz_type *multz) {
  DEBUG_ASSERT(multz)
  {
     multz_free_data(multz);
     free(multz);
  }
}


int multz_serialize(const multz_type *multz , double *serial_data , size_t stride , size_t offset) {
  DEBUG_ASSERT(multz);
  return mult_serialize(multz->mult , serial_data , stride , offset);
}


void multz_truncate(multz_type * multz) {
  DEBUG_ASSERT(multz)
  mult_truncate( multz->mult );  
}



void  multz_sample(multz_type *multz) {
  DEBUG_ASSERT(multz)
  mult_sample(multz->mult);  
}



multz_type * multz_alloc_mean(int ens_size , const multz_type **multz_ens) {
  int iens;
  multz_type * avg_multz = multz_copyc(multz_ens[0]);
  for (iens = 1; iens < ens_size; iens++) 
    multz_iadd(avg_multz , multz_ens[iens]);
  multz_iscale(avg_multz , 1.0 / ens_size);
  return avg_multz;
}





/*****************************************************************/


void multz_TEST() {
  const char * config_file = "/tmp/multz_config.txt";
  FILE * stream = util_fopen(config_file , "w");
  fprintf(stream , "1  0.01  0.01   0  NONE 1 11 1 11\n");
  fprintf(stream , "2  0.01  0.5    \n");
  fprintf(stream , "3 10.00 20.00   0  NONE 3 33 3 33\n");
  fclose(stream);
  
  {
    const int ens_size = 1000;
    char path[64];
    int iens;
    multz_config_type  * config    = multz_config_fscanf_alloc(config_file , 10, 10 ,10 , "MULTZ.INC" , NULL);
    multz_type        ** multz_ens = malloc(ens_size * sizeof * multz_ens);
    
    for (iens = 0; iens < ens_size; iens++) {
      multz_ens[iens] = multz_alloc(config);
      multz_sample(multz_ens[iens]);
      sprintf(path , "/tmp/%04d" , iens + 1);
      util_make_path(path);
      multz_ecl_write(multz_ens[iens] , path);
      multz_truncate(multz_ens[iens]);
    }
  }
}











MATH_OPS_MULT(multz)
VOID_ALLOC(multz)
VOID_FREE(multz)
VOID_FREE_DATA(multz)
VOID_REALLOC_DATA(multz)
VOID_ECL_WRITE (multz)
VOID_ENS_WRITE (multz)
VOID_ENS_READ  (multz)
VOID_COPYC     (multz)
VOID_SWAPIN(multz)
VOID_SWAPOUT(multz)
VOID_SERIALIZE(multz)
VOID_TRUNCATE(multz)
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC      (multz_clear        , multz_type)
VOID_FUNC      (multz_sample       , multz_type)


