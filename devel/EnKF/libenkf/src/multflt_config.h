#ifndef __MULTFLT_CONFIG_H__
#define __MULTFLT_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <config.h>

typedef struct multflt_config_struct multflt_config_type;

struct multflt_config_struct {
  CONFIG_STD_FIELDS;
  int    * logmode;
  char  ** fault_names;
  double * mean;
  double * std;
  bool   * active;
};


multflt_config_type * multflt_config_fscanf_alloc(const char * , const char * , const char * );
/*multflt_config_type * multflt_config_alloc(int , const char * , const char * );*/
void                  multflt_config_free(multflt_config_type *);
const          char * multflt_config_get_ensfile_ref(const multflt_config_type * );
const          char * multflt_config_get_eclfile_ref(const multflt_config_type * );



GET_SERIAL_SIZE_HEADER(multflt);
VOID_GET_SERIAL_SIZE_HEADER(multflt);
CONFIG_SET_ECLFILE_HEADER_VOID(multflt);
CONFIG_SET_ENSFILE_HEADER_VOID(multflt);
SET_SERIAL_OFFSET_HEADER(multflt);
VOID_SET_SERIAL_OFFSET_HEADER(multflt);
VOID_FUNC_HEADER(multflt_config_free);
#endif
