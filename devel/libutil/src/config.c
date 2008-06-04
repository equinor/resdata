#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <config_item.h>
#include <config.h>
#include <hash.h>

struct config_struct {
  hash_type  * items;
  bool         auto_add;
};




config_type * config_alloc(bool auto_add) {
  config_type *config = util_malloc(sizeof * config  , __func__);
  config->auto_add = auto_add;
  config->items    = hash_alloc();
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
  xreturn config_item_get_argv(item , &argc);
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



void config_parse(config_type * config , const char * filename, const char * comment_string) {
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
  fclose(stream);
}
