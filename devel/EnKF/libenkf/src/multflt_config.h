#ifndef __MULTFLT_CONFIG_H__
#define __MULTFLT_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>

typedef struct multflt_config_struct multflt_config_type;

struct multflt_config_struct {
  CONFIG_STD_FIELDS;
  bool   lognormal;
  char  ** fault_names;
  char   * ecl_file;
  char   * ens_file;
  double * mean;
  double * std;
  bool   * active;
};


multflt_config_type * multflt_config_alloc(int , const char * , const char * );
void                  multflt_config_free(multflt_config_type *);
const          char * multflt_config_get_ensname_ref(const multflt_config_type * );
const          char * multflt_config_get_eclname_ref(const multflt_config_type * );


GET_SIZE_FUNC_HEADER(multflt_config);
#endif
