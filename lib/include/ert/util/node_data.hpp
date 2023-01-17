#ifndef ERT_NODE_DATA_H
#define ERT_NODE_DATA_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(copyc_ftype)(const void *);
typedef void(free_ftype)(void *);

typedef struct node_data_struct node_data_type;

void node_data_free(node_data_type *);
void node_data_free_container(node_data_type *);
node_data_type *node_data_alloc_deep_copy(const node_data_type *);
node_data_type *node_data_alloc_shallow_copy(const node_data_type *);
node_data_type *node_data_alloc_copy(const node_data_type *node,
                                     bool deep_copy);
void *node_data_get_ptr(const node_data_type *);
const void *node_data_get_const_ptr(const node_data_type *);
node_data_type *node_data_alloc_buffer(const void *, int);
node_data_type *node_data_alloc_ptr(const void *, copyc_ftype *, free_ftype *);

node_data_type *node_data_alloc_int(int);
int node_data_get_int(const node_data_type *);
int node_data_fetch_and_inc_int(node_data_type *node_data);
node_data_type *node_data_alloc_double(double);
double node_data_get_double(const node_data_type *);
node_data_type *node_data_alloc_string(const char *);
char *node_data_get_string(const node_data_type *);

#ifdef __cplusplus
}
#endif
#endif
