#ifndef __ENKF_CONFIG_NODE_H__
#define __ENKF_CONFIG_NODE_H__
#include <enkf_types.h>

typedef void (config_free_ftype)                (void *);
typedef int  (config_get_serial_size_ftype)     (const void *);
typedef void (config_set_serial_offset_ftype)   (void * , int);

typedef struct enkf_config_node_struct enkf_config_node_type;

enkf_config_node_type * enkf_config_node_alloc(enkf_var_type     	          ,
					       enkf_impl_type    	          ,
					       const void        	        * ,
					       config_free_ftype 	        * ,
					       config_get_serial_size_ftype     * ,
                                               config_set_serial_offset_ftype   *);


void 		  enkf_config_node_free(enkf_config_node_type * );
void 		  enkf_config_node_free__(void * );
bool              enkf_config_node_include_type(const enkf_config_node_type * , int );
int  		  enkf_config_node_get_serial_size(enkf_config_node_type *, int *);
bool 		  enkf_config_node_include_type(const enkf_config_node_type * , int);
enkf_impl_type    enkf_config_node_get_impl_type(const enkf_config_node_type *);
const void     *  enkf_config_node_get_ref(const enkf_config_node_type * );

#endif
