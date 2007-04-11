#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <util.h>
#include <list_node.h>
#include <enkf_node.h>
#include <enkf_state.h>
#include <ens_config.h>
#include <ecl_config.h>


struct enkf_state_struct {
  list_type *nodes;
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
  enkf_state->nodes = list_alloc();
  return enkf_state;
}


static void enkf_state_add_node__(enkf_state_type * enkf_state , const enkf_node_type * node) {
  list_append_ref(enkf_state->nodes , node);
}


void enkf_state_add_node(enkf_state_type *enkf_state , void *data , ecl_read_ftype * ecl_read , ecl_write_ftype * ecl_write , ens_read_ftype *ens_read , ens_write_ftype * ens_write , sample_ftype *sample, free_ftype * free) {
  enkf_node_type *node = enkf_node_alloc(data , ecl_read , ecl_write , ens_read , ens_write , sample , freef);
  enkf_state_add_node__(enkf_state , enkf_node);
}


void enkf_state_ecl_write(const enkf_state_type * enkf_state) {
  list_node_type *list_node;
  list_node = list_get_head(enkf_state->nodes);
  while (list_node != NULL) {
    enkf_node_ecl_write(list_node_value_ptr(list_node));
    list_node = list_node_get_next(list_node);
  }
}


void enkf_state_ens_write(const enkf_state_type * enkf_state) {
  list_node_type *list_node;
  list_node = list_get_head(enkf_state->nodes);
  while (list_node != NULL) {
    enkf_node_ecl_write(list_node_value_ptr(list_node));
    list_node = list_node_get_next(list_node);
  }
}



char * enkf_state_config_alloc_ensname(const enkf_state_type * enkf_state, const char * ext_name) {
  char *path     = ens_config_alloc_ensname(enkf_state->ens_config , enkf_state->enkf_state_ens_path);
  char *ens_file = util_alloc_full_path(path , ext_name);

  util_make_path(path);
  free(path);

  return ens_file;
}


char * enkf_state_config_alloc_eclname(const enkf_state_type * enkf_state, const char * ext_name) {
  char *path     = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);
  char *ecl_file = util_alloc_full_path(path , ext_name);
  free(path);
  return ecl_file;
}


void enkf_state_config_make_ecl_path(const enkf_state_type *enkf_state) {
  char *path = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);
  util_make_path(path);
  free(path);
}


void enkf_state_config_unlink_ecl_path(const enkf_state_type *enkf_state) {
  char *path = ecl_config_alloc_eclname(enkf_state->ecl_config , enkf_state->enkf_state_ecl_path);
  util_unlink_path(path);
  free(path);
}




void enkf_state_config_free(enkf_state_type *enkf_state) {
  free(enkf_state->enkf_state_ens_path);
  free(enkf_state->enkf_state_ecl_path);
  free(enkf_state);
}

