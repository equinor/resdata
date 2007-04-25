#ifndef __ENKF_STATE_H__
#define __ENKF_STATE_H__

#include <fortio.h>
#include <stdbool.h>
#include <enkf_config.h>
#include <enkf_node.h>
#include <enkf_util.h>
#include <ecl_block.h>


typedef struct enkf_state_struct enkf_state_type;

char            * enkf_state_alloc_eclname(const enkf_state_type * , const char * );
char            * enkf_state_alloc_ensname(const enkf_state_type * , const char * );
void              enkf_state_add_node(enkf_state_type * , enkf_var_type , const char * , void *, ecl_write_ftype * , ens_read_ftype *, ens_write_ftype * , sample_ftype *, free_ftype * );
enkf_state_type  *enkf_state_alloc(const enkf_config_type * );
enkf_node_type  * enkf_state_get_node(const enkf_state_type * , const char * );
void              enkf_state_del_node(enkf_state_type * , const char * );

void              enkf_state_sample(enkf_state_type * , int);
void              enkf_state_ens_write(enkf_state_type * , int);
void              enkf_state_ens_read( enkf_state_type * , int);
void              enkf_state_ecl_write(enkf_state_type * , int , bool , bool , fortio_type *);
void              enkf_state_ecl_read(enkf_state_type * , const ecl_block_type *);
void              enkf_state_free(enkf_state_type * );
#endif
