#ifndef __MULTZ_CONFIG_H__
#define __MULTZ_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <config.h>
#include <enkf_types.h>

typedef struct {
  CONFIG_STD_FIELDS;
  int *i1, *i2 , *j1 , *j2 , *k;
  int *area;
  bool     lognormal;
  double * mean;
  double * std;
  bool   * active;
} multz_config_type;


multz_config_type * multz_config_alloc(int , int , int , const char * , const char * );
void                multz_config_free(multz_config_type *);
const        char * multz_config_get_ensfile_ref(const multz_config_type * );
const        char * multz_config_get_eclfile_ref(const multz_config_type * );
void                multz_config_fprintf_layer(const multz_config_type * , int , double , FILE *);


/*Generated headers */

GET_SIZE_HEADER(multz);
VOID_GET_SIZE_HEADER(multz);
CONFIG_SET_ECLFILE_HEADER_VOID(multz);
CONFIG_SET_ENSFILE_HEADER_VOID(multz);

VOID_FUNC_HEADER(multz_config_free);
#endif
