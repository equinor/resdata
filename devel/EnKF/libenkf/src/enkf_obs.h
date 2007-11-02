#ifndef __ENKF_OBS_H__
#define __ENKF_OBS_H__
#include <enkf_config.h>
#include <history.h>
#include <meas_data.h>
#include <well_obs.h>
#include <enkf_state.h>

#define WELL_OBS_TYPE_STRING  "WELL"
#define POINT_OBS_TYPE_STRING "POINT"

typedef struct enkf_obs_struct enkf_obs_type;


enkf_obs_type * enkf_obs_alloc(const enkf_config_type * , const history_type * );
void            enkf_obs_free(enkf_obs_type * );

void 		enkf_obs_measure(const enkf_obs_type * , int , const enkf_state_type * , meas_data_type * );
void 		enkf_obs_get_observations(enkf_obs_type * , int , obs_data_type * );
/*void            enkf_obs_add_well_obs(enkf_obs_type *  , const char * , const well_obs_type *);*/
enkf_obs_type * enkf_obs_fscanf_alloc(const char * , const enkf_config_type * , const history_type * );

#endif
