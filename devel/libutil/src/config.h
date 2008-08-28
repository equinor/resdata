#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include <stringlist.h>

#define ECL_COM_KW "--"
#define ENKF_COM_KW "--"


typedef struct config_struct      config_type;
typedef struct config_item_struct config_item_type;


char       ** 	  config_alloc_active_list(const config_type *, int *);
int           	  config_get_argc(const config_type *  , const char *);
const char ** 	  config_get_argv(const config_type *  , const char * , int *);
const char *  	  config_get(const config_type *  , const char *);
const char *  	  config_iget(const config_type *  , const char *, int);
void          	  config_free(config_type *);
config_type * 	  config_alloc( bool );
char       ** 	  config_alloc_active_list(const config_type * , int * );
void          	  config_parse(config_type * , const char * , const char * );
bool          	  config_has_item(const config_type * config , const char * kw);
void          	  config_set_arg(config_type * config , const char * , int , const char **);
stringlist_type * config_get_stringlist(const config_item_type * );

/*****************************************************************/

bool               config_item_set(const config_type * , const char * );
void               config_item_free__ (void * );
void               config_item_free( config_item_type * );
config_item_type * config_item_alloc(const char * , bool , bool);
config_item_type * config_get_item(const config_type *, const char *);
const char       * config_iget_arg(const config_item_type * , int);
int                config_item_get_argc(const config_item_type *);
const char      ** config_item_get_argv(const config_item_type * , int * );


const char * config_item_iget_argv(const config_item_type * , int );
bool 	   config_item_is_set(const config_item_type * );
void 	   config_item_set_arg(config_item_type *  , int , const char **);
int      config_item_get_argc(const config_item_type * );

config_item_type * config_add_item(config_type *, 
				   const char * ,
				   bool         ,
				   bool);


void config_parse(config_type *,
                  const char  *,
                  const char  *);

bool config_has_keys(const config_type *,
                     const char       **,
                     int                ,
                     bool               );

#endif
