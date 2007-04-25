#ifndef __MULTFLT_CONFIG_H__
#define __MULTFLT_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <config.h>

typedef struct multflt_config_struct multflt_config_type;

struct multflt_config_struct {
  CONFIG_STD_FIELDS;
  bool   lognormal;
  char  ** fault_names;
  double * mean;
  double * std;
  bool   * active;
};


multflt_config_type * multflt_config_alloc(int , const char * , const char * );
void                  multflt_config_free(multflt_config_type *);
const          char * multflt_config_get_ensname_ref(const multflt_config_type * );
const          char * multflt_config_get_eclname_ref(const multflt_config_type * );



CONFIG_GET_SIZE_FUNC_HEADER(multflt);
CONFIG_SET_ECL_FILE_HEADER_VOID(multflt);
CONFIG_SET_ENS_FILE_HEADER_VOID(multflt);

VOID_FUNC_HEADER(multflt_config_free);
#endif
