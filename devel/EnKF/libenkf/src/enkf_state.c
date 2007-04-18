#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include <hash.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <list_node.h>
#include <enkf_node.h>
#include <enkf_state.h>
#include <ens_config.h>
#include <ecl_config.h>


struct enkf_state_struct {
  list_type *node_list;
  hash_type *node_hash;
  const ens_config_type * ens_config;
  const ecl_config_type * ecl_config;
  char * enkf_state_ens_path; 
  char * enkf_state_ecl_path;
};


void enkf_state_set_ens_path(enkf_state_type * enkf_state, const char * ens_path) {
  enkf_state->enkf_state_ens_path = util_realloc_string_copy(enkf_state->enkf_state_ens_path , ens_path);
}


void enkf_state_set_ecl_path(enkf_state_type * enkf_state, const char * ecl_path) {
  enkf_state->enkf_state_ecl_path = util_realloc_string_copy(enkf_state->enkf_state_ecl_path , ecl_path);
}


enkf_state_type *enkf_state_alloc(const ens_config_type * ens_config , const ecl_config_type * ecl_config , const char *ens_path, const char *ecl_path) {
  enkf_state_type * enkf_state = malloc(sizeof *enkf_state);
  enkf_state->enkf_state_ens_path = NULL;
  enkf_state->enkf_state_ecl_path = NULL;

  enkf_state_set_ens_path(enkf_state , ens_path);
  enkf_state_set_ecl_path(enkf_state , ecl_path);

  enkf_state->ens_config = ens_config;
  enkf_state->ecl_config = ecl_config;
  enkf_state->node_list = list_alloc();
  enkf_state->node_hash = hash_alloc(10);
  return enkf_state;
}



/*
  hash_node -> list_node -> enkf_node -> {Actual enkf object: multz_type ...}
*/
static void enkf_state_add_node__(enkf_state_type * enkf_state , const char * node_key , const enkf_node_type * node) {
  list_node_type *list_node = list_append_ref(enkf_state->node_list , node);
  /*
    The hash contains a pointer to a list_node structure, which contain a pointer
    to an enkf_node which contains a pointer to the actual enkf object.
  */
  hash_insert_ref(enkf_state->node_hash , node_key  , list_node);
}



void enkf_state_add_node(enkf_state_type *enkf_state , const char * node_key , void *data , ecl_read_ftype * ecl_read , ecl_write_ftype * ecl_write , ens_read_ftype *ens_read , ens_write_ftype * ens_write , sample_ftype *sample, free_ftype * freef) {
  enkf_node_type *enkf_node = enkf_node_alloc(node_key , data , ecl_read , ecl_write , ens_read , ens_write , sample , freef);
  enkf_state_add_node__(enkf_state , node_key , enkf_node);
}


/*
void enkf_state_ecl_read(enkf_state_type * enkf_state , const ecl_block_type *ecl_block) {
  ecl_kw_type * ecl_kw = ecl_block_get_first_kw(ecl_block);
  while (ecl_kw != NULL) {
    ecl_kw = ecl_block_get_next_kw(ecl_block , ecl_kw);
    const char *kw     = ecl_kw_get_header_ref(ecl_kw);
    const bool enkf_kw = ecl_config_enkf_kw(enkf_state->ecl_config , kw);

    if (!enkf_state_has_node(enkf_state , kw)) {
      
    }
    
    

    if (enkf_kw) {

    } else {
      
    }
  }
}
*/


void enkf_state_ecl_write(const enkf_state_type * enkf_state) {
  list_node_type *list_node;
  list_node = list_get_head(enkf_state->node_list);
  while (list_node != NULL) {
    enkf_node_ecl_write(list_node_value_ptr(list_node));
    list_node = list_node_get_next(list_node);
  }
}


void enkf_state_ens_write(const enkf_state_type * enkf_state) {
  list_node_type *list_node;
  list_node = list_get_head(enkf_state->node_list);
  while (list_node != NULL) {
    enkf_node_ens_write(list_node_value_ptr(list_node));
    list_node = list_node_get_next(list_node);
  }
}

void enkf_state_sample(const enkf_state_type * enkf_state) {
  list_node_type *list_node;
  list_node = list_get_head(enkf_state->node_list);
  while (list_node != NULL) {
    enkf_node_sample(list_node_value_ptr(list_node));
    list_node = list_node_get_next(list_node);
  }
}



char * enkf_state_alloc_ensname(const enkf_state_type * enkf_state, const char * ext_name) {
  char *path     = ens_config_alloc_ensname(enkf_state->ens_config , enkf_state->enkf_state_ens_path);
  char *ens_file = util_alloc_full_path(path , ext_name);

  util_make_path(path);
  free(path);

  return ens_file;
}


char * enkf_state_alloc_eclname(const enkf_state_type * enkf_state, const char * ext_name) {
  char *path     = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);
  char *ecl_file = util_alloc_full_path(path , ext_name);
  free(path);
  return ecl_file;
}


void enkf_state_make_ecl_path(const enkf_state_type *enkf_state) {
  char *path = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);
  util_make_path(path);
  free(path);
}


void enkf_state_unlink_ecl_path(const enkf_state_type *enkf_state) {
  char *path = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);
  util_unlink_path(path);
  free(path);
}



void enkf_state_free(enkf_state_type *enkf_state) {
  free(enkf_state->enkf_state_ens_path);
  free(enkf_state->enkf_state_ecl_path);
  free(enkf_state);
}


bool enkf_state_has_node(const enkf_state_type * enkf_state , const char * node_key) {
  return hash_has_key(enkf_state->node_hash , node_key);
}


enkf_node_type * enkf_state_get_node(const enkf_state_type * enkf_state , const char * node_key) {
  if (hash_has_key(enkf_state->node_hash , node_key)) {
    list_node_type * list_node = hash_get(enkf_state->node_hash , node_key);
    enkf_node_type * enkf_node = list_node_value_ptr(list_node);
    return enkf_node;
  } else {
    fprintf(stderr,"%s: node:%s not found in state object - aborting \n",__func__ , node_key);
    abort();
  }
}



void enkf_state_del_node(enkf_state_type * enkf_state , const char * node_key) {
  if (hash_has_key(enkf_state->node_hash , node_key)) {
    list_node_type * list_node = hash_get(enkf_state->node_hash , node_key);
    enkf_node_type * enkf_node = list_node_value_ptr(list_node);
    
    list_del_node(enkf_state->node_list , list_node);
    hash_del(enkf_state->node_hash , node_key);
    enkf_node_free(enkf_node);  /* If a destructor is supplied - it is done here .*/
    
  } else {
    fprintf(stderr,"%s: node:%s not found in state object - aborting \n",__func__ , node_key);
    abort();
  } 
}


