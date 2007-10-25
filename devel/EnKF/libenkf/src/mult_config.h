#ifndef __MULT_CONFIG_H__
#define __MULT_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <config.h>
#include <logmode.h>

typedef struct mult_config_struct mult_config_type;




struct mult_config_struct {
  int data_size;            
  int serial_offset;        
  int internal_offset;
  logmode_type    ** logmode;
  double 	   * mean;
  double 	   * std;
  bool   	   * active;
  transform_ftype ** output_transform;
  char            ** output_transform_name;
};

mult_config_type *    mult_config_alloc_empty(int);
void                  mult_config_free(mult_config_type *);
const          char * mult_config_get_ensfile_ref(const mult_config_type * );
const          char * mult_config_get_eclfile_ref(const mult_config_type * );
void                  mult_config_transform(const mult_config_type * , const double * , double *);
void                  mult_config_truncate(const mult_config_type * config , double *);
void                  mult_config_fscanf_line(mult_config_type * , int , FILE * );
void                  mult_config_finalize_init(mult_config_type *config);


CONFIG_SET_ECLFILE_HEADER(mult);
CONFIG_SET_ENSFILE_HEADER(mult);
GET_DATA_SIZE_HEADER(mult);
CONFIG_SET_ECLFILE_HEADER_VOID(mult);
CONFIG_SET_ENSFILE_HEADER_VOID(mult);
SET_SERIAL_OFFSET_HEADER(mult);
GET_SERIAL_OFFSET_HEADER(mult);
VOID_SET_SERIAL_OFFSET_HEADER(mult);
VOID_FUNC_HEADER(mult_config_free);
#endif
