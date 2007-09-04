#ifndef __FIELD_CONFIG_H__
#define __FIELD_CONFIG_H__
#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <config.h>
#include <enkf_types.h>
#include <ecl_kw.h>


typedef struct field_config_struct field_config_type;

struct field_config_struct {
  CONFIG_STD_FIELDS;
  int nx,ny,nz;
  int sx,sy,sz;
  int logmode;
  const int *index_map;
  
  ecl_type_enum ecl_type;
  bool fmt_file;
  bool endian_swap;
};




void                field_config_get_dims(const field_config_type * , int * , int * , int *);
const int         * field_config_alloc_index_map2(int, int , int, const int *  , int *);
const int         * field_config_alloc_index_map1(const char * , bool , int *, int *, int *, int *);
field_config_type * field_config_alloc(const char * , ecl_type_enum ,  int , int , int , int , const int * , int , const char * , const char * );
void                field_config_free(field_config_type *);
const        char * field_config_get_ensfile_ref(const field_config_type * );
const        char * field_config_get_eclfile_ref(const field_config_type * );
void                field_config_set_io_options(const field_config_type * , bool *, bool * , ecl_type_enum *);
int                 field_config_get_volume(const field_config_type * );
void                field_config_set_ecl_kw_name(field_config_type * , const char * );
void                field_config_set_eclfile(field_config_type * , const char * );

/*Generated headers */
CONFIG_GET_ECL_KW_NAME_HEADER(field);
GET_SERIAL_SIZE_HEADER(field);
VOID_GET_SERIAL_SIZE_HEADER(field);
CONFIG_SET_ECLFILE_HEADER_VOID(field);
CONFIG_SET_ENSFILE_HEADER_VOID(field);
SET_SERIAL_OFFSET_HEADER(field);
VOID_SET_SERIAL_OFFSET_HEADER(field);
GET_SERIAL_OFFSET_HEADER(field);
VOID_FUNC_HEADER(field_config_free);
#endif
