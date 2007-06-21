#ifndef __ENKF_CONFIG_H__
#define __ENKF_CONFIG_H__
#include <stdbool.h>
#include <enkf_config_node.h>
#include <enkf_types.h>

#define LOG_INPUT    1
#define LOG_ENKF     2 
#define LOG_OUTPUT   4


/*****************************************************************/



typedef struct enkf_config_struct enkf_config_type;

bool               enkf_config_get_endian_swap(const enkf_config_type * );
int                enkf_config_get_eclpath_depth(const enkf_config_type * );
int                enkf_config_get_enspath_depth(const enkf_config_type * );
enkf_config_type * enkf_config_alloc(int , int , bool);
enkf_impl_type     enkf_config_impl_type(const enkf_config_type *, const char * );
bool               enkf_config_has_key(const enkf_config_type * , const char *);
void               enkf_config_add_type(enkf_config_type * , const char * , enkf_var_type , enkf_impl_type , const void * ,  config_free_ftype *, config_get_size_ftype *);
void               enkf_config_add_type0(enkf_config_type * , const char * , int , enkf_var_type , enkf_impl_type );
const void       * enkf_config_get_ref(const enkf_config_type * , const char * );
void               enkf_config_free(enkf_config_type * );
int                enkf_config_get_serial_size(const enkf_config_type * );

#endif
