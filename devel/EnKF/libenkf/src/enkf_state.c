#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include <hash.h>
#include <fortio.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <list_node.h>
#include <enkf_node.h>
#include <enkf_state.h>
#include <enkf_config.h>
#include <ecl_static_kw.h>
#include <enkf_ecl_kw.h>


struct enkf_state_struct {
  list_type *node_list;
  hash_type *node_hash;
  const enkf_config_type * config;
};



/* 
   Disse burde være vanlige (static) funksjoner - det blir for guffent ...
 */

/*****************************************************************/

#define ENKF_STATE_APPLY(node_func)                                     \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , int mask) { \
  list_node_type *list_node;                                            \
  list_node = list_get_head(enkf_state->node_list);                     \
  while (list_node != NULL) {                                           \
    enkf_node_type *enkf_node = list_node_value_ptr(list_node);         \
    if (enkf_node_include_type(enkf_node , mask))                       \
      enkf_node_ ## node_func (enkf_node);                              \
    list_node = list_node_get_next(list_node);                          \
  }                                                                     \
}

/*****************************************************************/

#define ENKF_STATE_APPLY2(node_func)                                     \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , const enkf_state_type *enkf_state2 , int mask) { \
  list_node_type *list_node;                                            \
  list_node_type *list_node2;                                           \
  list_node  = list_get_head(enkf_state->node_list);                    \
  list_node2 = list_get_head(enkf_state2->node_list);                   \
  while (list_node != NULL) {                                           \
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);        \
    enkf_node_type *enkf_node2 = list_node_value_ptr(list_node2);       \
    if (enkf_node_include_type(enkf_node , mask))                       \
      enkf_node_ ## node_func (enkf_node , enkf_node2);                 \
    list_node  = list_node_get_next(list_node);                         \
    list_node2 = list_node_get_next(list_node2);                        \
  }                                                                     \
}

/*****************************************************************/

#define ENKF_STATE_APPLY_SCALAR(node_func)                                     \
void enkf_state_ ## node_func(enkf_state_type * enkf_state , double scalar, int mask) { \
  list_node_type *list_node;                                            \
  list_node  = list_get_head(enkf_state->node_list);                    \
  while (list_node != NULL) {                                           \
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);        \
    if (enkf_node_include_type(enkf_node , mask))                       \
      enkf_node_ ## node_func (enkf_node , scalar);                     \
    list_node  = list_node_get_next(list_node);                         \
  }                                                                     \
}

/*****************************************************************/




enkf_state_type *enkf_state_alloc(const enkf_config_type * config , const char *ens_path, const char *ecl_path) {
  enkf_state_type * enkf_state = malloc(sizeof *enkf_state);
  
  enkf_state->config    = config;
  enkf_state->node_list = list_alloc();
  enkf_state->node_hash = hash_alloc(10);
  return enkf_state;
}



/*
  hash_node -> list_node -> enkf_node -> {Actual enkf object: multz_type/equil_type/multflt_type/...}
*/
static void enkf_state_add_node__(enkf_state_type * enkf_state , const char * node_key , const enkf_node_type * node) {
  list_node_type *list_node = list_append_ref(enkf_state->node_list , node);
  /*
    The hash contains a pointer to a list_node structure, which contain a pointer
    to an enkf_node which contains a pointer to the actual enkf object.
  */
  hash_insert_ref(enkf_state->node_hash , node_key  , list_node);
}



void enkf_state_add_node(enkf_state_type *enkf_state , enkf_var_type var_type , const char * node_key , void *data , ecl_read_ftype * ecl_read , ecl_write_ftype * ecl_write , ens_read_ftype *ens_read , ens_write_ftype * ens_write , sample_ftype *sample, free_ftype * freef) {
  enkf_node_type *enkf_node = enkf_node_alloc(node_key , var_type , data , ecl_read , ecl_write , ens_read , ens_write , sample , freef);
  enkf_state_add_node__(enkf_state , node_key , enkf_node);
}

static bool enkf_state_has_node(const enkf_state_type * enkf_state , const char * node_key) {
  return hash_has_key(enkf_state->node_hash , node_key);
}


void enkf_state_ecl_read(enkf_state_type * enkf_state , const ecl_block_type *ecl_block) {
  ecl_kw_type * ecl_kw = ecl_block_get_first_kw(ecl_block);
  while (ecl_kw != NULL) {
    const char *kw        = ecl_kw_get_header_ref(ecl_kw);
    const bool restart_kw = enkf_config_restart_kw(enkf_state->config , kw);

    if (!enkf_state_has_node(enkf_state , kw)) {
      if (restart_kw) {
	enkf_ecl_kw_type * new_kw = enkf_ecl_kw_alloc(enkf_state , kw , ecl_kw_get_size(ecl_kw) , kw);
	enkf_state_add_node(enkf_state , ecl_restart , kw , new_kw , NULL , NULL , NULL , NULL , NULL , NULL);
      } else {
	ecl_static_kw_type * new_kw = ecl_static_kw_alloc(enkf_state);
	enkf_state_add_node(enkf_state , ecl_static , kw , new_kw , NULL , NULL , NULL , NULL , NULL , NULL);
      }
    }

    {
      enkf_node_type * enkf_node = enkf_state_get_node(enkf_state , kw);
      
      if (restart_kw) 
	enkf_ecl_kw_get_data(enkf_node_value_ptr(enkf_node) , ecl_kw);    
      else 
	ecl_static_kw_init(enkf_node_value_ptr(enkf_node) , ecl_kw);
    }

    ecl_kw = ecl_block_get_next_kw(ecl_block , ecl_kw);
  }
}


void enkf_state_ecl_write(enkf_state_type * enkf_state , bool fmt_file , bool endian_swap , fortio_type * fortio ) {
  const int buffer_size = 16384;
  char *buffer;
  
  buffer = malloc(buffer_size);
  list_node_type *list_node;                                            
  list_node = list_get_head(enkf_state->node_list);                     
  while (list_node != NULL) {                                           
    list_node_type * next_node = list_node_get_next(list_node);
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);         

    if (enkf_node_include_type(enkf_node , ecl_restart)) {      
      ecl_kw_type * ecl_kw = enkf_ecl_kw_alloc_ecl_kw(enkf_node_value_ptr(enkf_node) , fmt_file , endian_swap);
      ecl_kw_fwrite(ecl_kw , fortio);
      ecl_kw_free(ecl_kw);
    } else if (enkf_node_include_type(enkf_node , ecl_static)) {
      char * ens_name      = enkf_state_alloc_ensname(enkf_state , enkf_node_get_key_ref(enkf_node));
      FILE * target_stream = fortio_get_FILE(fortio);
      FILE * src_stream    = fopen(ens_name , "r");
      
      util_copy_stream(src_stream , target_stream , buffer_size , buffer);
      fclose(src_stream);
      free(ens_name);
      
    } else if (enkf_node_include_type(enkf_node , parameter))
      enkf_node_ecl_write(enkf_node);
    
    list_node = next_node;
  } 
  free(buffer);

}




void enkf_state_make_ecl_path(const enkf_state_type *enkf_state) {
  /*char *path = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);*/

  printf("%s : BROKEN \n",__func__); abort();

  /*
    util_make_path(path);
  free(path);
  */
}


void enkf_state_unlink_ecl_path(const enkf_state_type *enkf_state) {
  /*char *path = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);*/

  printf("%s : BROKEN \n",__func__); abort();

  /*
    util_unlink_path(path);
    free(path);
  */
}


void enkf_state_free_nodes(enkf_state_type * enkf_state, int mask) {
  list_node_type *list_node;                                            
  list_node = list_get_head(enkf_state->node_list);                     
  while (list_node != NULL) {                                           
    list_node_type * next_node = list_node_get_next(list_node);
    enkf_node_type *enkf_node  = list_node_value_ptr(list_node);         

    if (enkf_node_include_type(enkf_node , mask))      
      enkf_state_del_node(enkf_state , enkf_node_get_key_ref(enkf_node));
    
    list_node = next_node;
  } 
}

void enkf_state_free(enkf_state_type *enkf_state) {
  enkf_state_free_nodes(enkf_state , all_types);

  free(enkf_state);
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


/*****************************************************************/
/* Generatad functions - iterating through all members.          */
/*****************************************************************/

ENKF_STATE_APPLY(ens_read);
ENKF_STATE_APPLY(ens_write);
ENKF_STATE_APPLY(sample);
ENKF_STATE_APPLY(clear);
ENKF_STATE_APPLY_SCALAR(scale);
ENKF_STATE_APPLY2(imul);
ENKF_STATE_APPLY2(iadd);
ENKF_STATE_APPLY2(iaddsqr);
