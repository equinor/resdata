#include <stdlib.h>
#include <string.h>
#include <enkf_types.h>
#include <util.h>
#include <field_config.h>
#include <enkf_macros.h>
#include <ecl_kw.h>


/*****************************************************************/


void field_config_set_ecl_kw_name(field_config_type * config , const char * ecl_kw_name) {
  config->ecl_kw_name = util_realloc_string_copy(config->ecl_kw_name , ecl_kw_name);
}



field_config_type * field_config_alloc(const char * ecl_kw_name , ecl_type_enum ecl_type , int nx , int ny , int nz , int active_size , const int * index_map , int logmode , const char * eclfile , const char * ensfile) {
  field_config_type *config = malloc(sizeof *config);
  
  /*
    Observe that size is the number of *ACTIVCE* cells,
    and generally *not* equal to nx*ny*nz.
  */
  config->data_size        = active_size; 

  
  config->ecl_kw_name = util_alloc_string_copy(ecl_kw_name);
  config->eclfile = NULL;
  config->ecl_kw_name = NULL;
  field_config_set_eclfile(config , eclfile);
  field_config_set_ecl_kw_name(config , ecl_kw_name);
  config->ensfile = util_alloc_string_copy(ensfile);
  config->logmode = logmode;

  config->nx = nx;
  config->ny = ny;
  config->nz = nz;

  config->sx = 1;
  config->sy = nx;
  config->sz = nx * ny;
  config->index_map = index_map;
  config->ecl_type  = ecl_type;
  
  config->fmt_file    = false;
  config->endian_swap = true;
  return config;
}


void field_config_set_io_options(const field_config_type * config , bool *fmt_file , bool * endian_swap, ecl_type_enum * ecl_type) {
  *fmt_file    = config->fmt_file;
  *endian_swap = config->endian_swap;
  *ecl_type    = config->ecl_type;
}


const char * field_config_get_ensfile_ref(const field_config_type * config) {
  return config->ensfile;
}

const char * field_config_get_eclfile_ref(const field_config_type * config) {
  return config->eclfile;
}


void field_config_free(field_config_type * config) {
  CONFIG_FREE_STD_FIELDS;
  free(config);
}
  

int field_config_get_volume(const field_config_type * config) {
  return config->nx * config->ny * config->nz;
}


const int * field_config_alloc_index_map2(int nx, int ny, int nz, const int * actnum  , int *active_size) {
  int index, active_index;
  int *index_map = enkf_util_calloc(nx * ny * nz , sizeof *index_map , __func__);
  active_index = 0;
  for (index = 0; index < nx*ny*nz; index++) {
    if (actnum[index] != 0) {
      index_map[index] = active_index;
      active_index++;
    } else
      index_map[index] = -1;
  }
  *active_size = active_index;
  return index_map;
}


/*int field_config_get_global_index(const field_config_type * field_config , int i , int j , int k) {
  
}
*/

void field_config_get_dims(const field_config_type * config , int *nx , int *ny , int *nz) {
  *nx = config->nx;
  *ny = config->ny;
  *nz = config->nz;
}


const int * field_config_alloc_index_map1(const char * EGRID_file , bool endian_flip , int *nx , int *ny , int *nz , int *active_size) {
  ecl_kw_type * actnum_kw , *ihead_kw;
  const int *index_map;
  bool fmt_file        = util_fmt_bit8(EGRID_file , 2 * 8192);

  fortio_type * fortio = fortio_open(EGRID_file , "r" , endian_flip);
  ecl_kw_fseek_kw("GRIDHEAD" , fmt_file , true , true , fortio);
  ihead_kw = ecl_kw_fread_alloc(fortio , fmt_file);

  ecl_kw_fseek_kw("ACTNUM" , fmt_file , true , true , fortio);
  actnum_kw = ecl_kw_fread_alloc(fortio , fmt_file);
  fortio_close(fortio);

  ecl_kw_iget(ihead_kw , 1 , nx);
  ecl_kw_iget(ihead_kw , 2 , ny);
  ecl_kw_iget(ihead_kw , 3 , nz);
  ecl_kw_free(ihead_kw);
  
  index_map = field_config_alloc_index_map2(*nx , *ny , *nz , ecl_kw_get_data_ref(actnum_kw) , active_size);
  ecl_kw_free(actnum_kw);

  return index_map;
}



/*****************************************************************/
CONFIG_GET_ECL_KW_NAME(field);
CONFIG_SET_ECLFILE(field);
CONFIG_SET_ENSFILE(field);
CONFIG_SET_ECLFILE_VOID(field);
CONFIG_SET_ENSFILE_VOID(field);
GET_DATA_SIZE(field)
VOID_FREE(field_config)
SET_SERIAL_OFFSET(field)
VOID_SET_SERIAL_OFFSET(field)
GET_SERIAL_OFFSET(field)
