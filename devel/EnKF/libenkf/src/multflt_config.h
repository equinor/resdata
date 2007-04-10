#ifndef __MULTFLT_CONFIG_H__
#define __MULTFLT_CONFIG_H__

#include <stdio.h>
#include <mem_config.h>

typedef struct multflt_config_struct multflt_config_type;

multflt_config_type * multflt_config_alloc(int , const char * , const char * );
void                  multflt_config_free(multflt_config_type *);
const          char * multflt_config_get_ensname_ref(const multflt_config_type * );
const          char * multflt_config_get_eclname_ref(const multflt_config_type * );
int                   multflt_config_get_nfaults(const multflt_config_type *);

#endif
