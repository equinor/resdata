#ifndef __ENKF_STATE_H__
#define __ENKF_STATE_H__

#include <ens_config.h>
#include <ecl_config.h>

typedef struct enkf_state_struct enkf_state_type;

enkf_state_type  *enkf_state_alloc(const ens_config_type * , const ecl_config_type * , const char *, const char *);
void              enkf_state_free(enkf_state_type * enkf_state);
void 		  enkf_state_add_node(enkf_state_type *  , const enkf_node_type * );
void 		  enkf_state_ecl_write(const enkf_state_type * );
#endif
