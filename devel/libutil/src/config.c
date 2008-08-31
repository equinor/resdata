#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <config.h>
#include <hash.h>
#include <stringlist.h>

/**
   Structure to parse configuration files of this type:

KEYWORD1  ARG2   ARG2  ARG3
KEYWORD2  ARG1-2
....
KEYWORDN  



Validating
----------
The config object implements three different ways of validating the input:

 1. If the xx__argc_minmax() function has been called, a line will not
    be accepted if the number of arguments is not within this range.

 2. If the type_map has been installed for the item (with the
    xx_arg_minmax function), it is checked the arguments of the item
    are in accordance with this typemap.

 3. If the item is added with required_set == true, the validate
    routine will fail if the item is not set.

Observe that the two first steps are checked when the item is parsed
(however the error is not reported before after the parsing is
complete),  whereas the last is checked when the parsing is
complete. Observe that is ABSOLUTELY ESSENTIAL that the final call
to config_parse() is with validate == true, otherwise the validation 
will not be performed / acted upon.
*/



struct config_struct {
  hash_type  	  * items;             	       /* A hash of config_items - the actual content. */
  bool       	    auto_add;          	       /* Whether unknown items should be added */
  bool       	    append_arg_default_value;  /* When we dynamically add a new item - should that be initialized with append_arg == true ? */
  stringlist_type * parse_errors;              /* A stringlist containg the errors found when parsing.*/
  int               error_count;               /* The number of errors found when parsing. */
};


#define __TYPE__ 6751

struct config_item_struct {
  int                         __id;   			 /* Used for run-time checking */
  char                        * kw;   			 /* The kw which identifies this item· */
  stringlist_type             * stringlist;     	 /* The values which have been set. */
  bool                          append_arg;     	 /* Should the values be appended if a keyword appears several times in the config file. */
  bool                          currently_set;           /* Has a value been assigned to this keyword. */
  bool                          required_set;            
  stringlist_type             * selection_set;           /* A list of strings which the value(s) must match (can be NULL) */
  stringlist_type             * required_children;       /* A list of item's which must also be set (if this item is set). (can be NULL) */
  int                           argc_min;                /* The minimum number of arguments for this keyword -1 means no lower limit. */
  int                           argc_max;                /* The maximum number of arguments for this keyword (on one line) -1 means no limit. */   
  config_item_types           * type_map;                /* A list of types for the items - can be NULL. Set along with argc_minmax(); */
};

/*****************************************************************/




config_item_type * config_item_alloc(const char * kw , bool required , bool append_arg) {
  config_item_type * item = util_malloc(sizeof * item , __func__);

  item->__id       = __TYPE__;
  item->kw         = util_alloc_string_copy(kw);
  item->stringlist = stringlist_alloc_new();

  item->currently_set 	  = false;
  item->append_arg    	  = append_arg;
  item->required_set  	  = required;
  item->argc_min          = -1;  /* -1 - not applicable */
  item->argc_max          = -1;
  item->selection_set     = NULL;
  item->required_children = NULL;
  item->type_map          = NULL;
  return item;
}


static void config_add_error(config_type * config , const char * error_message) {
  if (error_message != NULL) {
    config->error_count++;
    stringlist_append_owned_ref(config->parse_errors , util_alloc_sprintf("%02d: %s" , config->error_count , error_message));
  }
}


/*
  The last argument (config_file) is only used for printing
  informative error messages, and can be NULL.

  Returns a string with an error description, or NULL if the supplied
  arguments were OK. The string is allocated here, but is assumed that
  calling scope will free it.
*/

static char * config_item_append_arg(config_item_type * item , int argc , const char ** argv , const char * config_file) {
  char * error_message = NULL;
  int iarg;
  bool OK;
  
  if (item->argc_min >= 0) {
    if (argc < item->argc_min) {
      OK = false;
      
      if (config_file != NULL)
	error_message = util_alloc_sprintf("Error when parsing config_file:\"%s\" Keyword:%s must have at least %d arguments.",config_file , item->kw , item->argc_min);
      else
	error_message = util_alloc_sprintf("Error:: Keyword:%s must have at least %d arguments.",item->kw , item->argc_min);
    }
  }

  if (item->argc_max >= 0) {
    if (argc > item->argc_max) {
      OK = false;
      if (config_file != NULL)
	error_message = util_alloc_sprintf("Error when parsing config_file:\"%s\" Keyword:%s must have maximum %d arguments.",config_file , item->kw , item->argc_min);
      else
	error_message = util_alloc_sprintf("Error:: Keyword:%s must have maximum %d arguments.",item->kw , item->argc_min);
    }
  }

  for (iarg = 0; iarg < argc; iarg++) {
    bool OK = true;

    if (item->selection_set != NULL) {
      if (!stringlist_contains(item->selection_set , argv[iarg])) {
	error_message = util_alloc_sprintf("%s: is not a valid value for: %s.",argv[iarg] , item->kw);
	OK = false;
      } 
    }
    if (OK) stringlist_append_copy(item->stringlist , argv[iarg]);
  }
  return error_message;
}


char * config_item_set_arg(config_item_type * item , int argc , const char **argv , const char * config_file) {
  if (!item->append_arg) 
    stringlist_clear(item->stringlist);
  
  item->currently_set = true;
  return config_item_append_arg(item , argc , argv , config_file);
}



void config_item_validate(config_type * config , const config_item_type * item) {
  if (item->required_set && !item->currently_set) {
    char * error_message = util_alloc_sprintf("Item:%s must be set with a value.",item->kw);
    config_add_error(config , error_message);
    free(error_message);
  }
}


int config_item_get_argc(const config_item_type * item ) {
  return stringlist_get_argc(item->stringlist);
}


const char ** config_item_get_argv(const config_item_type * item , int * argc) {
  *argc = stringlist_get_argc(item->stringlist);
  return stringlist_get_argv(item->stringlist);
}

stringlist_type * config_get_stringlist(const config_item_type * item) {
  return item->stringlist;
}



const char * config_item_iget_argv(const config_item_type * item , int iarg) {
  return stringlist_iget(item->stringlist , iarg);
}




void config_item_free( config_item_type * item) {
  free(item->kw);
  stringlist_free(item->stringlist);
  if (item->required_children != NULL) stringlist_free(item->required_children);
  if (item->selection_set != NULL)     stringlist_free(item->selection_set);
  util_safe_free(item->type_map);
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

void config_item_set_selection_set(config_item_type * item , const stringlist_type * stringlist) {
  if (item->selection_set != NULL)
    stringlist_free(item->selection_set);
  
  item->selection_set = stringlist_alloc_deep_copy(stringlist);
}

void config_item_add_to_selection(config_item_type * item , const char *value) {
  if (item->selection_set == NULL)
    item->selection_set = stringlist_alloc_new();
  stringlist_append_copy(item->selection_set , value);
}



void config_item_set_required_children(config_item_type * item , stringlist_type * stringlist) {
  item->required_children = stringlist_alloc_deep_copy(stringlist);
}


/**
   This function is used to set the minimum and maximum number of
   arguments for an item. In addition you can pass in a pointer to an
   array of config_item_types values which will be used for validation
   of the input. This vector must be argc_max elements long; it can be
   NULL.
*/


void config_item_set_argc_minmax(config_item_type * item , int argc_min , int argc_max, const config_item_types * type_map) {
  item->argc_min = argc_min;
  item->argc_max = argc_max;

  util_safe_free(item->type_map);
  if (type_map != NULL)
    item->type_map = util_alloc_copy(type_map , argc_max * sizeof * type_map , __func__);
  
}
  


#undef __TYPE__



/*****************************************************************/



config_type * config_alloc(bool auto_add) {
  config_type *config 		   = util_malloc(sizeof * config  , __func__);
  config->auto_add    		   = auto_add;
  config->items       		   = hash_alloc();
  config->append_arg_default_value = false;
  config->parse_errors             = stringlist_alloc_new();
  config->error_count              = 0;
  return config;
}


void config_free(config_type * config) {
  hash_free(config->items);
  stringlist_free(config->parse_errors);
  free(config);
}




/**
   This function allocates a simple item with all values
   defaulted. The item is added to the config object, and a pointer is
   returned to the calling scope. If you want to change the properties
   of the item you can do that with config_item_set_xxxx() functions
   from the calling scope.
*/


config_item_type * config_add_item(config_type * config , 
				   const char  * kw, 
				   bool  required  , 
				   bool  append_arg) {
  
  config_item_type * item = config_item_alloc( kw , required , append_arg);
  hash_insert_hash_owned_ref(config->items , kw , item , config_item_free__);
  
  return item;
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

void config_set_arg(config_type * config , const char * kw, int argc , const char **argv) {
  char * error_message = config_item_set_arg(config_get_item(config , kw) , argc , argv , NULL);
  config_add_error(config , error_message);
}


/**
   Can take argc == NULL
*/
const char ** config_get_argv(const config_type * config , const char *kw , int *argc) {
  int local_argc;
  config_item_type * item = config_get_item(config , kw);
  
  if (argc == NULL)
    return config_item_get_argv(item , &local_argc);
  else
    return config_item_get_argv(item , argc);

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



const char * config_iget(const config_type * config , const char * kw, int iarg) {
  config_item_type * item = config_get_item(config , kw);
  const int argc = config_item_get_argc(item);
  if (argc < iarg) 
    util_abort("%s: (internal ?) error when calling %s, trying to fetch item out of bounds. %s has %i items, trying to fetch item %i (zero offset).\n",__func__,__func__,kw,argc,iarg);
  else if(iarg < 0)
    util_abort("%s: (internal ?) error when calling %s, trying to fetch item out of bounds, item requested was %i.\n ",__func__,__func__,iarg);

  return config_item_iget_argv(item , iarg);
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




static void config_validate(config_type * config, const char * filename) {
  int size = hash_get_size(config->items);
  char ** key_list = hash_alloc_keylist(config->items);
  int ikey;
  for (ikey = 0; ikey < size; ikey++) {
    const config_item_type * item = config_get_item(config , key_list[ikey]);
    config_item_validate(config , item);
  }
  util_free_stringlist(key_list , size);
  if (config->error_count > 0) {
    stringlist_fprintf(config->parse_errors , "\n", stderr);
    util_exit("");
  }
}


void config_parse(config_type * config , const char * filename, const char * comment_string , bool validate) {
  FILE * stream = util_fopen(filename , "r");
  bool   at_eof = false;
  
  while (!at_eof) {
    int i , tokens;
    int active_tokens;
    char **token_list;
    char  *line;
    
    line  = util_fscanf_alloc_line(stream , &at_eof);
    if (line != NULL) {
      util_split_string(line , " \t" , &tokens , &token_list);
      
	active_tokens = tokens;
	for (i = 0; i < tokens; i++) {
	  char * comment_ptr = NULL;
	  if(comment_string != NULL)
	    comment_ptr = strstr(token_list[i] , comment_string);
	  
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
	  if (!config_has_item(config , kw) && config->auto_add) 
	    config_add_item(config , kw , false , config->append_arg_default_value);

	  if (config_has_item(config , kw)) {
	    config_item_type * item = config_get_item(config , kw);
	    config_item_set_arg(item , active_tokens - 1, (const char **) &token_list[1] , filename);
	  } else 
	    fprintf(stderr,"** Warning keyword:%s not recognized when parsing:%s - ignored \n",kw,filename);
	  
	}
	util_free_stringlist(token_list , tokens);
	free(line);
    }
  }
  if (validate) config_validate(config , filename);
  fclose(stream);
}



bool config_has_keys(const config_type * config, const char **ext_keys, int ext_num_keys, bool exactly)
{
  int i;

  int     config_num_keys;
  char ** config_keys;

  config_keys = config_alloc_active_list(config, &config_num_keys);

  if(exactly && (config_num_keys != ext_num_keys))
  {
    util_free_stringlist(config_keys,config_num_keys);
    return false;
  }

  for(i=0; i<ext_num_keys; i++)
  {
    if(!config_has_item(config,ext_keys[i]))
    {
      util_free_stringlist(config_keys,config_num_keys);
      return false;
    }
  }
 
  util_free_stringlist(config_keys,config_num_keys);
  return true;
}
