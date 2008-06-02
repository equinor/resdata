#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <config_item.h>


struct config_item_struct {
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
};



config_item_type * config_item_alloc(const char * kw) {
  config_item_type * item = util_malloc(sizeof * item , __func__);

  item->kw   = util_alloc_string_copy(kw);
  item->argc = 0;
  item->argv = NULL;

  item->currently_set = false;
  item->append_arg    = false;
  item->required_set  = false;
  item->required_children = NULL;
  item->num_children      = 0;
  item->validator         = NULL;
  
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
  }
  config_item_append_arg(item , argc , argv);
  item->currently_set = true;
}


void config_item_init(config_item_type * item, 
		      int default_argc  , const char ** default_argv , 
		      bool required_set , 
		      bool append_arg   ,
		      int num_children  , const char ** required_children) {
  item->required_set = required_set;
  item->append_arg   = append_arg;
  if (default_argc > 0)
    config_item_set_arg(item , default_argc , default_argv);
}



void config_item_free( config_item_type * item) {
  free(item->kw);
  util_free_stringlist(item->argv              , item->argc);
  util_free_stringlist(item->required_children , item->num_children);
  free(item);
}

  
