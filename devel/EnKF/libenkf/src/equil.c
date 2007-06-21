#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <util.h>
#include <enkf_types.h>
#include <equil_config.h>
#include <equil.h>
#include <enkf_util.h>


struct equil_struct {
  const equil_config_type *config;
  double                  *data;
};

/*****************************************************************/

void equil_free_data(equil_type * equil) {
  free(equil->data);
  equil->data = NULL;
}

void equil_realloc_data(equil_type * equil) {
  equil->data  = enkf_util_realloc(equil->data , equil->config->size * sizeof *equil->data , __func__);
}

equil_type * equil_alloc(const equil_config_type * config) {
  equil_type * equil    = malloc(sizeof *equil);
  equil->config         = config;
  equil->data           = NULL;
  equil_realloc_data(equil);
  return equil;
}


equil_type * equil_copyc(const equil_type * src) {
  equil_type * new = equil_alloc(src->config);
  memcpy(new->data , src->data , equil_config_get_size(new->config) * sizeof * new->data);
  return new;
}




void equil_ecl_write(const equil_type * equil, const char * path) {
  char * eclfile = util_alloc_full_path(path , equil_config_get_eclfile_ref(equil->config));
  FILE * stream   = enkf_util_fopen_w(eclfile , __func__);
  {
    const int size = equil->config->size;
    int k;
    for (k=0; k < size; k++)
      /*config_fprintf_layer(equil->config , k + 1 , equil->data[k] , stream);*/
      fprintf(stream , "FAULT:%3d ... %g \n",k,equil->data[k]);
  }
  free(eclfile);
  fclose(stream);
}


static char * equil_alloc_ensfile(const equil_type * equil, const char * path) {
  return util_alloc_full_path(path , equil_config_get_ensfile_ref(equil->config));
}


void equil_fwrite(const equil_type * equil, const char *file) {
  const  equil_config_type * config = equil->config;
  FILE * stream  = enkf_util_fopen_w(file , __func__);
  fwrite(&config->size  , sizeof  config->size     , 1 , stream);
  enkf_util_fwrite(equil->data    , sizeof *equil->data    , config->size , stream , __func__);
  fclose(stream);
}

void equil_ens_write(const equil_type * equil, const char * path) {
  char * ensfile = equil_alloc_ensfile(equil , path);
  equil_fwrite(equil , ensfile);
  free(ensfile);
}

char * equil_swapout(equil_type * equil , const char * path) {
  char * ensfile = equil_alloc_ensfile(equil , path);
  equil_fwrite(equil , ensfile);
  equil_free_data(equil);
  return ensfile;
}


void equil_fread(equil_type * equil , const char *file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  int  nz;
  fread(&nz , sizeof  nz     , 1 , stream);
  enkf_util_fread(equil->data , sizeof *equil->data , nz , stream , __func__);
  fclose(stream);
}

void equil_swapin(equil_type * equil , const char *file) {
  equil_realloc_data(equil);
  equil_fread(equil , file);
}


void equil_ens_read(equil_type * equil , const char * path) {
  char * ensfile = equil_alloc_ensfile(equil , path);
  equil_fread(equil , ensfile);
  free(ensfile);
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
  equil_free_data(equil);
  free(equil);
}


void equil_serialize(const equil_type *equil , double *serial_data , size_t *_offset) {
  printf("Warning:%s is not properly implemented ... \n",__func__);
}




VOID_SWAPOUT(equil);
VOID_SWAPIN(equil);

MATH_OPS(equil);
VOID_ALLOC(equil);
VOID_SERIALIZE (equil);
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/
VOID_ECL_WRITE (equil)
VOID_ENS_WRITE (equil)
VOID_ENS_READ  (equil)
VOID_COPYC     (equil)
VOID_FUNC      (equil_sample    , equil_type)
VOID_FUNC      (equil_free      , equil_type)

