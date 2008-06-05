#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <config.h>
#include <hash.h>

struct config_struct {
  int          parse_count;
  hash_type  * items;
  bool         auto_add;
};

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
  char                       ** selection_set;
  int                           selection_size;  
  char                       ** required_children;
  int                           num_children;
  config_item_validate_ftype  * validator;
  int                           argc_min;  /* Observe that these are NOT for the total - but pr line */
  int                           argc_max;
};

/*****************************************************************/




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
  item->selection_set     = NULL;
  item->selection_size    = 0;
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
  bool OK;
  if (item->argc_min >= 0) {
    if (argc < item->argc_min) {
      OK = false;
      fprintf(stderr,"Error when parsing. Keyword:%s must have at least %d arguments \n",item->kw , item->argc_min);
    }
  }

  if (item->argc_max >= 0) {
    if (argc > item->argc_max) {
      OK = false;
      fprintf(stderr , "Error when parsing. Keyword:%s must have maximum %d arguments \n",item->kw , item->argc_max);
    }
  }

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


bool config_item_validate(const config_type * config , const config_item_type * item) {
  bool OK = true;
  if (item->required_set && !item->currently_set) {
    fprintf(stderr , "**ERROR: item:%s has not been set \n",item->kw);
    OK = false;
  } else {
    if (item->validator != NULL) 
      OK = item->validator(config , item);
  }
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
  util_free_stringlist(item->selection_set     , item->selection_size);
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



/*****************************************************************/



config_type * config_alloc(bool auto_add) {
  config_type *config = util_malloc(sizeof * config  , __func__);
  config->auto_add    = auto_add;
  config->items       = hash_alloc();
  config->parse_count = 0;
  return config;
}


void config_free(config_type * config) {
  hash_free(config->items);
  free(config);
}


void config_add_item(config_type * config, const char * kw , const config_item_type * item) {
  hash_insert_hash_owned_ref(config->items , kw , item , config_item_free__);
}


void config_init_item(config_type * config , 
		      const char * kw, 
		      int default_argc  , const char ** default_argv , 
		      bool required_set , 
		      bool append_arg   ,
		      int num_children  , const char ** required_children,
		      int argc_min      , int argc_max,
		      config_item_validate_ftype * validator) {

  config_item_type * item = config_item_alloc( kw );
  config_item_init(item , default_argc , default_argv , required_set , append_arg , num_children , required_children , argc_min , argc_max , validator);
  config_add_item(config , kw , item);
}


bool config_has_item(const config_type * config , const char * kw) {
  return hash_has_key(config->items , kw);
}

config_item_type * config_get_item(const config_type * config , const char * kw) {
  return hash_get(config->items , kw);
}

bool config_item_set(const config_type * config , const char * kw) {
  return config_item_is_set(hash_get(config->items , kw));
}


const char ** config_get_argv(const config_type * config , const char *kw) {
  int argc;
  config_item_type * item = config_get_item(config , kw);
  return config_item_get_argv(item , &argc);
}


int config_get_argc(const config_type * config , const char * kw) {
  config_item_type * item = config_get_item(config , kw);
  return config_item_get_argc(item);
}



const char * config_get(const config_type * config , const char * kw) {
  config_item_type * item = config_get_item(config , kw);
  const int argc = config_item_get_argc(item);
  if (argc != 1) 
    util_abort("%s: (internal ?) error when calling %s the keyword must have *ONE* argument. %s had %d \n",__func__ , __func__ , kw , argc);
  return config_item_iget_argv(item , 0);
}


char ** config_alloc_active_list(const config_type * config, int * _active_size) {
  char ** complete_key_list = hash_alloc_keylist(config->items);
  char ** active_key_list = NULL;
  int complete_size = hash_get_size(config->items);
  int active_size   = 0;
  int i;

  for( i = 0; i < complete_size; i++) {
    if  (config_item_is_set(config_get_item(config , complete_key_list[i]) )) {
      active_key_list = util_stringlist_append_copy(active_key_list , active_size , complete_key_list[i]);
      active_size++;
    }
  }
  *_active_size = active_size;
  util_free_stringlist(complete_key_list , complete_size);
  return active_key_list;
}




static void config_validate(const config_type * config, const char * filename) {
  int size = hash_get_size(config->items);
  char ** key_list = hash_alloc_keylist(config->items);
  int ikey;
  bool OK = true;
  for (ikey = 0; ikey < size; ikey++) {
    const config_item_type * item = config_get_item(config , key_list[ikey]);
    OK = config_item_validate(config , item);
  }
  util_free_stringlist(key_list , size);
  if (!OK) 
    util_exit("There were errors when parsing configuration file:%s \n",filename);
  
}


void config_parse(config_type * config , const char * filename, const char * comment_string) {
  if (config->parse_count > 0) 
    util_abort("%s: Sorry config_parse can only be called once on one config instance\n",__func__);
  {  
    FILE * stream = util_fopen(filename , "r");
    bool   at_eof = false;
    
    while (!at_eof) {
      int i , tokens;
      int active_tokens;
      char **token_list;
      char  *line;
      
      line  = util_fscanf_alloc_line(stream , &at_eof);
      if (line != NULL) {
	util_split_string(line , " " , &tokens , &token_list);
	
	active_tokens = tokens;
	for (i = 0; i < tokens; i++) {
	  char * comment_ptr = strstr(token_list[i] , comment_string);
	  if (comment_ptr != NULL) {
	    if (comment_ptr == token_list[i])
	      active_tokens = i;
	    else
	      active_tokens = i + 1;
	    break;
	  }
	}
	if (active_tokens > 0) {
	  const char * kw = token_list[0];
	  if (!config_has_item(config , kw) && config->auto_add) {
	    config_item_type * new_item = config_item_alloc(kw);
	    config_add_item(config , kw , new_item);
	  }
	  if (config_has_item(config , kw)) {
	    config_item_type * item = config_get_item(config , kw);
	    config_item_set_arg(item , active_tokens - 1, (const char **) &token_list[1]);
	  } else 
	    fprintf(stderr,"** Warning keyword:%s not recognized when parsing:%s - ignored \n",kw,filename);
	  
	}
      }
    }
    config_validate(config , filename);
    fclose(stream);
  }
}


