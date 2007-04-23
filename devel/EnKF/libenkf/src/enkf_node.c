#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <enkf_node.h>
#include <util.h>


struct enkf_node_struct {
  ecl_write_ftype *ecl_write;
  ecl_read_ftype  *ecl_read;
  ens_read_ftype  *ens_read;
  ens_write_ftype *ens_write;
  sample_ftype    *sample;
  free_ftype      *freef;
  copyc_ftype     *copyc;
  char            *node_key;
  void            *data;
};



enkf_node_type * enkf_node_alloc(const char *node_key, void *data , ecl_read_ftype * ecl_read , ecl_write_ftype * ecl_write , ens_read_ftype *ens_read , ens_write_ftype * ens_write , sample_ftype *sample, free_ftype * freef) {
  enkf_node_type *node = malloc(sizeof *node);
  node->ecl_write = ecl_write;
  node->ecl_read  = ecl_read;
  node->ens_read  = ens_read;
  node->ens_write = ens_write;
  node->sample    = sample;
  node->freef     = freef;
  node->data      = data;
  node->node_key  = util_alloc_string_copy(node_key);
  return node;
}

#define FUNC_ASSERT(func,func_name) if (func == NULL) { fprintf(stderr,"%s: function handler: %s not registered when writing node:%s - aborting\n",__func__ , func_name , enkf_node->node_key); abort(); }

void * enkf_node_value_ptr(const enkf_node_type * enkf_node) { return enkf_node->data; }

void enkf_node_ecl_write(const enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->ecl_write , "ecl_write");
  enkf_node->ecl_write(enkf_node->data);
}

void enkf_node_ens_write(const enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->ens_write , "ens_write");
  enkf_node->ens_write(enkf_node->data);
}

void enkf_node_ens_read(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->ens_read , "ens_read");
  enkf_node->ens_read(enkf_node->data);
}


/*
  void enkf_node_scale(enkf_node_type *enkf_node , double scale_factor) {
  FUNC_ASSERT(enkf_node->scale , "scale");
  enkf_node->scale(enkf_node->data , scale_factor);
  }
*/


void enkf_node_sample(enkf_node_type *enkf_node) {
  FUNC_ASSERT(enkf_node->sample , "sample");
  enkf_node->sample(enkf_node->data);
}

void enkf_node_free(enkf_node_type *enkf_node) {
  if (enkf_node->freef != NULL)
    enkf_node->freef(enkf_node->data);
  free(enkf_node->node_key);
  free(enkf_node);
}

#undef FUNC_ASSERT




