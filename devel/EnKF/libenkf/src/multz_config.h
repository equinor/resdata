#ifndef __MULTZ_CONFIG_H__
#define __MULTZ_CONFIG_H__
#include <mem_config.h>

typedef struct multz_config_struct multz_config_type;

multz_config_type * multz_config_alloc(int , int , int , const char * , const char * );
void                multz_config_free(multz_config_type *);
const        char * multz_config_get_ensname_ref(const multz_config_type * );

#endif
