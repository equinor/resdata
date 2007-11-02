#ifndef __MULTFLT_CONFIG_H__
#define __MULTFLT_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <enkf_macros.h>
#include <logmode.h>
#include <mult_config.h>
#include <mult.h>

typedef struct multflt_config_struct multflt_config_type;

struct multflt_config_struct {
  char              * ecl_kw_name;      
  enkf_var_type       var_type;  
  char 		    * ensfile;          
  char 		    * eclfile;          
  mult_config_type  * mult_config;
  char             ** fault_names;
};


multflt_config_type * multflt_config_fscanf_alloc(const char * , const char * , const char * );
/*multflt_config_type * multflt_config_alloc(int , const char * , const char * );*/
void                  multflt_config_free(multflt_config_type *);
void                  multflt_config_transform(const multflt_config_type * , const double * , double *);
void                  multflt_config_truncate(const multflt_config_type * , mult_type * );
int                   multflt_config_get_data_size(const multflt_config_type * );

CONFIG_GET_ENSFILE_HEADER(multflt);
CONFIG_GET_ECLFILE_HEADER(multflt);
CONFIG_SET_ENSFILE_HEADER(multflt);
CONFIG_SET_ECLFILE_HEADER(multflt);
CONFIG_SET_ECLFILE_HEADER_VOID(multflt);
CONFIG_SET_ENSFILE_HEADER_VOID(multflt);
SET_SERIAL_OFFSET_HEADER(multflt);
VOID_SET_SERIAL_OFFSET_HEADER(multflt);
VOID_FUNC_HEADER(multflt_config_free);
#endif
