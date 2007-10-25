#ifndef __EQUIL_CONFIG_H__
#define __EQUIL_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <config.h>

typedef struct equil_config_struct equil_config_type;

struct equil_config_struct {
  CONFIG_STD_FIELDS;
  double * mean_WOC;
  double * std_WOC;
  bool   * active_WOC;
  double * mean_GOC;
  double * std_GOC;
  bool   * active_GOC;
};

int                   equil_config_get_nequil(const equil_config_type * );
equil_config_type   * equil_config_alloc(int , bool , bool , const char * , const char * );
void                  equil_config_free(equil_config_type *);
const          char * equil_config_get_ensfile_ref(const equil_config_type * );
const          char * equil_config_get_eclfile_ref(const equil_config_type * );
int                   equil_config_get_nequil(const equil_config_type *);


VOID_ALLOC_ENSFILE_HEADER(equil);
CONFIG_SET_ECLFILE_HEADER_VOID(equil);
CONFIG_SET_ENSFILE_HEADER_VOID(equil);
SET_SERIAL_OFFSET_HEADER(equil);
VOID_SET_SERIAL_OFFSET_HEADER(equil);

VOID_FUNC_HEADER(equil_config_free);

#endif
