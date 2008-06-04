#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <config_item.h>


#define __TYPE__ 6751

struct config_item_struct {
  int     __id;
  char  * kw;
  int     argc;
  char  **argv;
  /**************************/
  bool                          currently_set;
  bool                          append_arg;
  bool                          required_set;
  char                       ** required_children;
  int                           num_children;
  config_item_validate_ftype  * validator;
  int                           argc_min;
  int                           argc_max;
};



config_item_type * config_item_alloc(const char * kw) {
  config_item_type * item = util_malloc(sizeof * item , __func__);

  item->__id = __TYPE__;
  item->kw   = util_alloc_string_copy(kw);
  item->argc = 0;
  item->argv = NULL;

  item->currently_set = false;
  item->append_arg    = false;
  item->required_set  = false;
  item->required_children = NULL;
  item->num_children      = 0;
  item->validator         = NULL;
  item->argc_min          = -1;  /* -1 - not applicable */
  item->argc_max          = -1;
  return item;
}


static void config_item_append_arg(config_item_type * item , int argc , const char ** argv) {
  int iarg;
  item->argv = util_realloc( item->argv , (argc + item->argc) * sizeof * item->argv , __func__);
  for (iarg = 0; iarg < argc; iarg++)
    item->argv[iarg + item->argc] = util_alloc_string_copy(argv[iarg]);
  item->argc += argc;
}


void config_item_set_arg(config_item_type * item , int argc , const char **argv) {
  if (!item->append_arg) {
    util_free_stringlist(item->argv , item->argc);
    item->argc = 0;
    item->argv = NULL;
  }
  config_item_append_arg(item , argc , argv);
  item->currently_set = true;
}


void config_item_init(config_item_type * item, 
		      int default_argc  , const char ** default_argv , 
		      bool required_set , 
		      bool append_arg   ,
		      int num_children  , const char ** required_children,
		      int argc_min      , int argc_max,
		      config_item_validate_ftype * validator) {
  item->required_set = required_set;
  item->append_arg   = append_arg;
  item->validator    = validator;
  if (default_argc > 0)
    config_item_set_arg(item , default_argc , default_argv);
  item->num_children = num_children;
  item->required_children = util_alloc_stringlist_copy(required_children , num_children);
  item->argc_min = argc_min;
  item->argc_max = argc_max;
}  


bool config_item_validate(const config_item_type * item) {
  bool OK = true;
  if (item->argc_min >= 0) {
    if (item->argc < item->argc_min) {
      OK = false;
      fprintf(stderr,"Error when parsing. Keyword:%s must have at least %d arguments \n",item->kw , item->argc_min);
    }
  }

  if (item->argc_max >= 0) {
    if (item->argc > item->argc_max) {
      OK = false;
      fprintf(stderr , "Error when parsing. Keyword:%s must have maximum %d arguments \n",item->kw , item->argc_max);
    }
  }
  /*
    if (item->validator != NULL) 
    OK = OK && item->validator();
  */
  return OK;
}

int config_item_get_argc(const config_item_type * item ) {
  return item->argc;
}

const char ** config_item_get_argv(const config_item_type * item , int * argc) {
  *argc = item->argc;
  return (const char **) item->argv;
}

const char * config_item_iget_argv(const config_item_type * item , int iarg) {
  if (item->argc > iarg)
    return item->argv[iarg];
  else {
    util_abort("%s: asked for argument nr:%d - the keyword only has %d arguments \n",__func__ , iarg + 1 , item->kw , item->argc);
    return NULL;
  }
}




void config_item_free( config_item_type * item) {
  free(item->kw);
  util_free_stringlist(item->argv              , item->argc);
  util_free_stringlist(item->required_children , item->num_children);
  free(item);
}

void config_item_free__ (void * void_item) {
  config_item_type * item = (config_item_type *) void_item;
  if (item->__id != __TYPE__) 
    util_abort("%s: internal error - cast failed \n",__func__);
  
  config_item_free( item );
}


bool config_item_is_set(const config_item_type * item) {
  return item->currently_set;
}

  
#undef __TYPE__
