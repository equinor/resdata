#ifndef __EQUIL_CONFIG_H__
#define __EQUIL_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <enkf_util.h>
#include <enkf_config.h>
#include <scalar_config.h>

typedef struct equil_config_struct equil_config_type;

struct equil_config_struct {
  CONFIG_STD_FIELDS;

  double * datum_depth;
  double * datum_P;
  double * oil_water_Pc;
  double * gas_oil_Pc;
  int    * live_oil_init_mode;
  int    * black_oil_wgas_init_mode;
  int    * init_accuracy;

  scalar_config_type * scalar_config;
};

void                  equil_config_ecl_write(const equil_config_type *   , const double *  , const double * , FILE * );
int                   equil_config_get_nequil(const equil_config_type * );
equil_config_type   * equil_config_alloc(int , const char * , const char * );
void                  equil_config_free(equil_config_type *);
const          char * equil_config_get_ensfile_ref(const equil_config_type * );
const          char * equil_config_get_eclfile_ref(const equil_config_type * );
int                   equil_config_get_nequil(const equil_config_type *);
equil_config_type   * equil_config_fscanf_alloc(const char * , const char *  , const char *);

VOID_ALLOC_ENSFILE_HEADER(equil);
CONFIG_SET_ECLFILE_HEADER_VOID(equil);
CONFIG_SET_ENSFILE_HEADER_VOID(equil);
SET_SERIAL_OFFSET_HEADER(equil);
VOID_SET_SERIAL_OFFSET_HEADER(equil);

VOID_FUNC_HEADER(equil_config_free);

#endif
