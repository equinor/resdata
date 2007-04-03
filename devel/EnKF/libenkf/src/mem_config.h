#ifndef __MEM_CONFIG_H__
#define __MEM_CONFIG_H__
#include <ens_config.h>

typedef struct mem_config_struct mem_config_type;


mem_config_type  * mem_config_alloc(const ens_config_type * , const char * , const char * );
char             * mem_config_alloc_ensname(const mem_config_type * , const char * );
void               mem_config_free(mem_config_type *);
void               mem_config_set_ens_path(mem_config_type * , const char * );
void               mem_config_set_ecl_path(mem_config_type * , const char * );

#endif
