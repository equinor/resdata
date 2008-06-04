#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct config_struct config_type;



int           config_get_argc(const config_type *  , const char *);
const char ** config_get_argv(const config_type *  , const char *);
const char *  config_get(const config_type *  , const char *);
void          config_free(config_type *);
config_type * config_alloc( bool );

#endif
