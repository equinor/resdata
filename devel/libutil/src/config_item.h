#ifndef __CONFIG_ITEM_H__
#define __CONFIG_ITEM_H__

typedef struct config_item_struct config_item_type;


typedef bool ( config_item_validate_ftype ) (const char * , 
					     int   argc   , 
					     const char ** argv);


void               config_item_free( config_item_type * );
config_item_type * config_item_alloc(const char * );

#endif
