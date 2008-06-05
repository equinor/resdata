#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct config_struct      config_type;
typedef struct config_item_struct config_item_type;

typedef bool ( config_item_validate_ftype ) (const config_type *, 
					     const config_item_type *);

int           config_get_argc(const config_type *  , const char *);
const char ** config_get_argv(const config_type *  , const char *);
const char *  config_get(const config_type *  , const char *);
void          config_free(config_type *);
config_type * config_alloc( bool );

/*****************************************************************/


void               config_item_free__ (void * );
void               config_item_free( config_item_type * );
config_item_type * config_item_alloc(const char * );
const char       * config_iget_arg(const config_item_type * , int);
int                config_item_get_argc(const config_item_type *);
const char      ** config_item_get_argv(const config_item_type * , int * );

void config_item_init(config_item_type * ,
		      int ,
		      const char ** ,
		      bool ,
		      bool ,
		      int ,
		      const char ** ,
		      int , int, 
		      config_item_validate_ftype * );

const char * config_item_iget_argv(const config_item_type * , int );
bool 	   config_item_is_set(const config_item_type * );
void 	   config_item_set_arg(config_item_type *  , int , const char **);
int        config_item_get_argc(const config_item_type * );

#endif
