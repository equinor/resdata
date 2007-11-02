#ifndef __OBS_NODE__
#define __OBS_NODE__
#include <config.h>
#include <enkf_types.h>
#include <obs_data.h>
#include <meas_data.h>
#include <time.h>

typedef void (obs_free_ftype)                (void *);
typedef void (obs_get_ftype)                 (const void * , int , obs_data_type *);
typedef void (obs_meas_ftype)                (const void * , const void *, meas_data_type *);

typedef struct obs_node_struct obs_node_type;


void            obs_node_measure(const obs_node_type *  , int , const void * , meas_data_type * );
obs_node_type * obs_node_alloc(const void * , enkf_active_type , time_t , obs_get_ftype * , obs_meas_ftype * , obs_free_ftype *);
void            obs_node_free(obs_node_type * );
const void *    obs_node_get_ref(const obs_node_type * );
void            obs_node_get_observations(obs_node_type * , int , obs_data_type * );


VOID_FREE_HEADER(obs_node);

#endif
