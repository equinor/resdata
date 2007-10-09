#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <util.h>
#include <enkf_types.h>
#include <equil_config.h>
#include <equil.h>
#include <enkf_util.h>


GET_DATA_SIZE_HEADER(equil);

/*****************************************************************/

/* 
   Storage is only in data.
*/

struct equil_struct {
  const equil_config_type *config;
  double                  *data;
  double                  *data_GOC;
  double                  *data_WOC;
};

/*****************************************************************/

void equil_free_data(equil_type * equil) {
  free(equil->data);
  equil->data = NULL;
}



void equil_realloc_data(equil_type * equil) {
  const int data_size = equil_config_get_data_size(equil->config);
  equil->data         = enkf_util_realloc(equil->data , data_size * sizeof *equil->data , __func__);
  equil->data_WOC     = equil->data;
  equil->data_GOC     = &equil->data[equil_config_get_nequil(equil->config)];
}


equil_type * equil_alloc(const equil_config_type * config) {
  equil_type * equil    = malloc(sizeof *equil);
  equil->config         = config;
  equil->data           = NULL;
  equil->data_WOC       = NULL;
  equil->data_GOC       = NULL;
  equil_realloc_data(equil);
  return equil;
}


equil_type * equil_copyc(const equil_type * src) {
  equil_type * new = equil_alloc(src->config);
  memcpy(new->data_WOC , src->data_WOC , equil_config_get_nequil(new->config) * sizeof * new->data_WOC);
  memcpy(new->data_GOC , src->data_GOC , equil_config_get_nequil(new->config) * sizeof * new->data_GOC);
  return new;
}




void equil_ecl_write(const equil_type * equil, const char * path) {
  char * eclfile = util_alloc_full_path(path , equil_config_get_eclfile_ref(equil->config));
  FILE * stream   = enkf_util_fopen_w(eclfile , __func__);
  {
    const int nequil = equil_config_get_nequil(equil->config);
    int k;
    for (k=0; k < nequil; k++) {
      /*config_fprintf_layer(equil->config , k + 1 , equil->data[k] , stream);*/
      fprintf(stream , "FAULT:%3d ... %g \n",k,equil->data_WOC[k]);
      fprintf(stream , "FAULT:%3d ... %g \n",k,equil->data_GOC[k]);
    }
  }
  free(eclfile);
  fclose(stream);
}


static char * equil_alloc_ensfile(const equil_type * equil, const char * path) {
  return util_alloc_full_path(path , equil_config_get_ensfile_ref(equil->config));
}


void equil_fwrite(const equil_type * equil, const char *file) {
  const int data_size = equil_config_get_data_size(equil->config);
  FILE * stream       = enkf_util_fopen_w(file , __func__);
  
  fwrite(&data_size  , sizeof  data_size     , 1 , stream);
  enkf_util_fwrite(equil->data ,  sizeof *equil->data    , data_size , stream , __func__);
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
  int data_size;
  fread(&data_size , sizeof  data_size  , 1 , stream);
  enkf_util_fread(equil->data , sizeof *equil->data , data_size , stream , __func__);
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
  const bool              *active_WOC   = config->active_WOC;
  const double            *std_WOC      = config->std_WOC;
  const double            *mean_WOC     = config->mean_WOC;
  const bool              *active_GOC   = config->active_GOC;
  const double            *std_GOC      = config->std_GOC;
  const double            *mean_GOC     = config->mean_GOC;
  const int                nequil       = equil_config_get_nequil(config);
  int i;
  
  for (i=0; i < nequil; i++) {
    if (active_WOC[i]) 
      equil->data_WOC[i] = enkf_util_rand_normal(mean_WOC[i] , std_WOC[i]);
    if (active_GOC[i]) 
      equil->data_GOC[i] = enkf_util_rand_normal(mean_GOC[i] , std_GOC[i]);
  }
  
}


void equil_free(equil_type *equil) {
  equil_free_data(equil);
  free(equil);
}


static int equil_serialize_component(int nequil , const double * data , const bool * active, double * serial_data , size_t stride , size_t offset) {
  int active_size = 0;
  int i;

  for (i=0; i < nequil; i++) 
    if (active[i]) {
      serial_data[offset + i*stride] = data[i];
      active_size++;
    }
  return active_size;
}


int equil_serialize(const equil_type *equil , double *serial_data , int ens_size , size_t offset) {
  const equil_config_type *config       = equil->config;
  const bool              *active_WOC   = config->active_WOC;
  const bool              *active_GOC   = config->active_GOC;
  const double            *data_WOC     = equil->data_WOC;
  const double            *data_GOC     = equil->data_GOC;
  int   nequil                          = equil_config_get_nequil(config);
  int active_size;

  active_size  = equil_serialize_component(nequil , data_WOC , active_WOC , serial_data , ens_size , offset);
  active_size += equil_serialize_component(nequil , data_GOC , active_GOC , serial_data , ens_size , active_size + offset);
  
  return active_size;
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

