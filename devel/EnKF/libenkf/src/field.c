#include <math.h>
#include <stdlib.h>
#include <enkf_config.h>
#include <field.h>
#include <util.h>
#include <string.h>
#include <fortio.h>
#include <ecl_kw.h>
#include <field_config.h>
#include <rms_file.h>
#include <rms_tagkey.h>


#define  DEBUG
#define  TARGET_TYPE FIELD
#include "enkf_debug.h"


static inline int __global_index(const field_config_type * config , int i , int j , int k) {
  return config->index_map[ k * config->nx * config->ny + j * config->nx + i];
}

GET_DATA_SIZE_HEADER(field);

/*****************************************************************/

struct field_struct {
  DEBUG_DECLARE
  const field_config_type * config;
  double * data;
};


#define EXPORT_MACRO                                                                           \
for (k=0; k < config->nz; k++) {                                                               \
  for (j=0; j < config->ny; j++) {                                                             \
    for (i=0; i < config->nx; i++) {                                                           \
      int index1D = __global_index(config , i , j , k);                                        \
      int index3D;                                                                             \
      if (rms_order)                                               		     	       \
        index3D = i * config->ny*config->nz  +  j * config->ny + (config->nz - k);             \
      else                                                                       	       \
        index3D = i + j * config->nx + k* config->nx*config->ny;           	               \
      if (index1D >= 0)                                                                        \
	data[index3D] = field->data[index1D];                               	               \
      else                                                                                     \
	data[index3D] = fill_value;                                                            \
     }                                                                                         \
  }                                                                                            \
}


static void field_export3D(const field_type * field , void *_data , bool rms_order , bool export_float , double fill_value) {
  const field_config_type * config = field->config;
  int i,j,k;
  
  if (export_float) {
    float *data = (float *) _data;
    EXPORT_MACRO
 } else {
    double *data = (double *) _data;
    EXPORT_MACRO
  }  

}
  

/*****************************************************************/


#define IMPORT_MACRO                                                                           \
for (k=0; k < config->nz; k++) {                                                               \
  for (j=0; j < config->ny; j++) {                                                             \
    for (i=0; i < config->nx; i++) {                                                           \
      int index1D = __global_index(config , i , j , k);                                        \
      int index3D;                                                                             \
      if (index1D >= 0) {                                                                      \
	if (rms_order)                                               		     	       \
	  index3D = i * config->ny*config->nz  +  j * config->ny + (config->nz - k);           \
	else                                                                       	       \
	  index3D = i + j * config->nx + k* config->nx*config->ny;           	               \
	field->data[index1D] = data[index3D] ;                               	               \
     }                                                                                         \
   }                                                                                           \
  }                                                                                            \
}


static void field_import3D(field_type * field , const void *_data , bool rms_order , bool import_float) {
  const field_config_type * config = field->config;
  int i,j,k;
  
  if (import_float) {
    const float *data = (const float *) _data;
    IMPORT_MACRO
  } else {
    const double *data = (const double *) _data;
    IMPORT_MACRO
  }  
}


/*****************************************************************/


void field_clear(field_type * field) {
  const int data_size = field_config_get_data_size(field->config);   
  int k;
  for (k = 0; k < data_size; k++)
    field->data[k] = 0.0;
}


void field_realloc_data(field_type *field) {
  field->data = enkf_util_calloc(field_config_get_data_size(field->config) , sizeof *field->data , __func__);
}


void field_free_data(field_type *field) {
  free(field->data);
  field->data = NULL;
}


field_type * field_alloc(const field_config_type * field_config) {
  field_type * field  = malloc(sizeof *field);
  field->config = field_config;
  field->data = NULL;
  field_realloc_data(field);
  DEBUG_ASSIGN(field)
  return field;
}



char * field_alloc_ensfile(const field_type * field , const char * path) {
  return util_alloc_full_path(path , field_config_get_ensfile_ref(field->config));
}


field_type * field_copyc(const field_type *field) {
  const int data_size = field_config_get_data_size(field->config);   
  field_type * new = field_alloc(field->config);
  
  memcpy(new->data , field->data , data_size * sizeof *field->data);
  return new;
}


void field_fread(field_type * field , const char *file) {
  FILE * stream   = enkf_util_fopen_r(file , __func__);
  int  data_size;
  fread(&data_size , sizeof  data_size     , 1 , stream);
  enkf_util_fread(field->data , sizeof *field->data , data_size , stream , __func__);
  fclose(stream);
}


void field_fwrite(const field_type * field , const char *file) {
  const int data_size = field_config_get_data_size(field->config);
  FILE * stream   = enkf_util_fopen_w(file , __func__);

  fwrite(&data_size                    ,   sizeof  data_size     , 1 , stream);
  enkf_util_fwrite(field->data    , sizeof *field->data    , data_size , stream , __func__);
  
  fclose(stream);
}





void field_ecl_write1D_fortio(const field_type * field , fortio_type * fortio , bool fmt_file , bool endian_swap , ecl_type_enum ecl_type) {
  const int data_size = field_config_get_data_size(field->config);
  void *data;
  if (ecl_type == ecl_float_type) {
    data = enkf_util_calloc(data_size , sizeof(float) , __func__);
    util_double_to_float(data , field->data , data_size );
  } else 
    data = field->data;

  ecl_kw_fwrite_param_fortio(fortio , fmt_file , endian_swap , field_config_get_ecl_kw_name(field->config), ecl_type , data_size , data);
  if (ecl_type == ecl_float_type) 
    free(data);
}



void field_ecl_write3D_fortio(const field_type * field , fortio_type * fortio , bool fmt_file , bool endian_swap , ecl_type_enum ecl_type) {
  const int data_size = field_config_get_volume(field->config);
  void *data;
  bool export_float;
  data = enkf_util_calloc(data_size , sizeof(float) , __func__);
  if (ecl_type == ecl_float_type) 
    export_float = true;
  else
    export_float = false;
  field_export3D(field , data , false , export_float , 0.0);

  ecl_kw_fwrite_param_fortio(fortio , fmt_file , endian_swap , field_config_get_ecl_kw_name(field->config), ecl_type , data_size , data);
  free(data);
}


void field_ecl_write2(const field_type * field  , const char * path , bool write3D) {
  fortio_type * fortio;
  bool fmt_file , endian_swap;
  ecl_type_enum ecl_type;
  char * eclfile = util_alloc_full_path(path , field_config_get_eclfile_ref(field->config));

  field_config_set_io_options(field->config , &fmt_file , &endian_swap, &ecl_type);
  fortio = fortio_open(eclfile , "w" , endian_swap);

  if (write3D)
    field_ecl_write3D_fortio(field , fortio , fmt_file , endian_swap , ecl_type);
  else
    field_ecl_write1D_fortio(field , fortio , fmt_file , endian_swap , ecl_type);

  fortio_close(fortio);
  free(eclfile);
}




void field_ecl_write3D(const field_type * field , const char * path) {
  field_ecl_write2(field , path , true);
}


void field_ecl_write1D(const field_type * field , const char * path) {
  field_ecl_write2(field , path , false);
}



void field_ecl_write(const field_type * field , const char * path) {
  field_ecl_write1D(field , path);
}



void field_ens_read(field_type * field , const char *path) {
  char * ensfile = util_alloc_full_path(path , field_config_get_ensfile_ref(field->config));
  field_fread(field , ensfile);
  free(ensfile);
}


void field_ens_write(const field_type * field , const char * path) {
  char * ensfile = util_alloc_full_path(path , field_config_get_ensfile_ref(field->config));
  field_fwrite(field , ensfile);
  free(ensfile);
}


char * field_swapout(field_type * field , const char * path) {
  char * ensfile = util_alloc_full_path(path , field_config_get_ensfile_ref(field->config));
  field_fwrite(field , ensfile);
  field_free_data(field);
  return ensfile;
}


void field_swapin(field_type * field , const char *file) {
  field_realloc_data(field);
  field_fread(field  , file);
}






void field_sample(field_type *field) {
  printf("%s: Warning not implemented ... \n",__func__);
}




void field_free(field_type *field) {
  field_free_data(field);
  free(field);
}



int field_deserialize(const field_type * field , int internal_offset , size_t serial_size , const double * serial_data , size_t stride , size_t offset) {
  const field_config_type *config      = field->config;
  const int                data_size  = field_config_get_data_size(config);

  return enkf_util_deserialize(field->data , NULL , internal_offset , data_size , serial_size , serial_data , offset , stride);
}




int field_serialize(const field_type *field , int internal_offset , size_t serial_data_size ,  double *serial_data , size_t stride , size_t offset , bool *complete) {
  const field_config_type *config      = field->config;
  const int                data_size  = field_config_get_data_size(config);
  
  return enkf_util_serialize(field->data , NULL , internal_offset , data_size , serial_data , serial_data_size , offset , stride , complete);
}




/*
  int index05D = config->index_map[ k * config->nx * config->ny + j * config->nx + i];      2
*/



double field_ijk_get(const field_type * field , int i , int j , int k) {
  int global_index = __global_index(field->config , i , j , k);
  return field->data[global_index];
}


bool field_ijk_valid(const field_type * field , int i , int j , int k) {
  int global_index = __global_index(field->config , i , j , k);
  if (global_index >=0)
    return true;
  else
    return false;
}

double field_ijk_get_if_valid(const field_type * field , int i , int j , int k , bool * valid) {
  int global_index = __global_index(field->config , i , j , k);
  if (global_index >=0) {
    *valid = true;
    return field->data[global_index];
  } else {
    *valid = false;
    return 0.0;
  }
}


int field_get_global_index(const field_type * field , int i , int j  , int k) {
  return __global_index(field->config , i , j , k);
}


void field_copy_ecl_kw_data(field_type * field , const ecl_kw_type * ecl_kw) {
  const field_config_type * config = field->config;
  const int data_size = field_config_get_data_size(config);
  if (data_size != ecl_kw_get_size(ecl_kw)) {
    fprintf(stderr,"%s: fatal error - incorrect size for:%s [config:%d , file:%d] - aborting \n",__func__ , config->ecl_kw_name , data_size , ecl_kw_get_size(ecl_kw));
    abort();
  } 
  {
    void *data;
    ecl_type_enum ecl_type = config->ecl_type;
    if (ecl_type == ecl_float_type) 
      data = enkf_util_calloc(data_size , sizeof(float) , __func__ );
    else if (ecl_type == ecl_double_type)
      data = field->data;
    else {
      fprintf(stderr,"%s: field->ecl_type = %d is not recognized - aborting \n",__func__ , ecl_type);
      abort();
    }
    ecl_kw_get_memcpy_data(ecl_kw , data);
    if (ecl_type == ecl_float_type) {
      util_float_to_double(field->data , data , data_size);
      free(data);
    }
  }
}





/* Skal param_name vaere en variabel ?? */
void field_rms_export_parameter(const field_type * field , const char * param_name , const float * data3D,  const rms_file_type * rms_file) {
  const field_config_type * config = field->config;
  const int data_size = field_config_get_data_size(config);
  
  /* Hardcoded rms_float_type */
  rms_tagkey_type *tagkey = rms_tagkey_alloc_complete("data" , data_size , rms_float_type , data3D , true);
  rms_tag_fwrite_parameter(param_name , tagkey , rms_file_get_FILE(rms_file));
  rms_tagkey_free(tagkey);
  
}

void field_get_dims(const field_type * field, int *nx, int *ny , int *nz) {
  field_config_get_dims(field->config , nx , ny ,nz);
}


MATH_OPS(field)
VOID_ALLOC(field)
VOID_FREE(field)
VOID_FREE_DATA(field)
VOID_REALLOC_DATA(field)
VOID_ECL_WRITE (field)
VOID_ENS_WRITE (field)
VOID_ENS_READ  (field)
VOID_COPYC     (field)
VOID_SWAPIN(field)
VOID_SWAPOUT(field)
VOID_SERIALIZE (field);
VOID_DESERIALIZE (field);
/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

VOID_FUNC      (field_clear        , field_type)
VOID_FUNC      (field_sample       , field_type)



