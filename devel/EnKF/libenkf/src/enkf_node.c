#include <string.h>
#include <stdlib.h>
#include <enkf_node.h>
#include <util.h>


struct enkf_node_struct {
  ecl_write_ftype *ecl_write;
  ecl_read_ftype  *ecl_read;
  ens_read_ftype  *ens_read;
  ens_write_ftype *ens_write;
  sample_ftype    *sample;
  void            *data;
};


enkf_node_type * enkf_node_alloc(void *data , ecl_read_ftype * ecl_read , ecl_write_ftype * ecl_write , ens_read_ftype *ens_read , ens_write_ftype * ens_write , sample_ftype *sample) {
  enkf_node_type *node = malloc(sizeof *node);
  node->ecl_write = ecl_write;
  node->ecl_read  = ecl_read;
  node->ens_read  = ens_read;
  node->ens_write = ens_write;
  node->sample    = sample;
  node->data      = data;
  return node;
}


void enkf_node_ecl_write(const enkf_node_type *enkf_node) {
  enkf_node->ecl_write(enkf_node->data);
}

void enkf_node_ens_write(const enkf_node_type *enkf_node) {
  enkf_node->ens_write(enkf_node->data);
}

void enkf_node_ens_read(enkf_node_type *enkf_node) {
  enkf_node->ens_read(enkf_node->data);
}


void enkf_node_sample(enkf_node_type *enkf_node) {
  enkf_node->sample(enkf_node->data);
}


void enkf_node_free(enkf_node_type *enkf_node) {
  free(enkf_node);
}




