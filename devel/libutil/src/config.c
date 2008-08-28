#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <config.h>
#include <hash.h>
#include <stringlist.h>

struct config_struct {
  int          parse_count;
  hash_type  * items;
  bool         auto_add;
  bool         append_arg_default_value;
};

#define __TYPE__ 6751


struct config_item_struct {
  int                         __id;   			 /* Used for run-time checking */
  char                        * kw;   			 /* The kw which identifies this item· */
  stringlist_type             * stringlist;     	 /* The values which have been set. */
  bool                          append_arg;     	 /* Should the values be appended if a keyword appears several times in the config file. */
  bool                          currently_set;           /* Has a value been assigned to this keyword. */
  bool                          required_set;            
  stringlist_type             * selection_set;           /* A list of strings which the value(s) must match */
  stringlist_type             * required_children;       /* A list of item's which must also be set (if this item is set). */
  int                           argc_min;                /* Observe that these are NOT for the total - but pr line */
  int                           argc_max;
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
  return item;
}


static void config_item_append_arg(config_item_type * item , int argc , const char ** argv) {
  int iarg;
  for (iarg = 0; iarg < argc; iarg++) {
    if (item->selection_set != NULL) 
      if (!stringlist_contains(item->selection_set , argv[iarg])) {
	fprintf(stderr,"Valid values for:%s : ",item->kw);
	stringlist_fprintf(item->selection_set , stderr);
	util_abort("%s: value:%s is not valid for key:%s \n",__func__ , argv[iarg] , item->kw);
      }
    stringlist_append_copy(item->stringlist , argv[iarg]);
  }
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

  if (!item->append_arg) 
    stringlist_clear(item->stringlist);
  
  config_item_append_arg(item , argc , argv);
  item->currently_set = true;
}


bool config_item_validate(const config_type * config , const config_item_type * item) {
  bool OK = true;
  if (item->required_set && !item->currently_set) {
    fprintf(stderr , "**ERROR: item:%s has not been set \n",item->kw);
    OK = false;
  } 
  return OK;
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

void config_item_set_selection_set(config_item_type * item , stringlist_type * stringlist) {
  item->selection_set = stringlist_alloc_deep_copy(stringlist);
}

void config_item_set_required_children(config_item_type * item , stringlist_type * stringlist) {
  item->required_children = stringlist_alloc_deep_copy(stringlist);
}

void config_item_set_argc_minmax(config_item_type * item , int argc_min , int argc_max) {
  item->argc_min = argc_min;
  item->argc_max = argc_max;
}
  


#undef __TYPE__



/*****************************************************************/



config_type * config_alloc(bool auto_add) {
  config_type *config 		   = util_malloc(sizeof * config  , __func__);
  config->auto_add    		   = auto_add;
  config->items       		   = hash_alloc();
  config->parse_count 		   = 0;         
  config->append_arg_default_value = false;
  return config;
}


void config_free(config_type * config) {
  hash_free(config->items);
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
  config_item_set_arg(config_get_item(config , kw) , argc , argv);
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
	    config_item_set_arg(item , active_tokens - 1, (const char **) &token_list[1]);
	  } else 
	    fprintf(stderr,"** Warning keyword:%s not recognized when parsing:%s - ignored \n",kw,filename);
	}
	util_free_stringlist(token_list , tokens);
	free(line);
      }
    }
    config_validate(config , filename);
    fclose(stream);
  }
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
