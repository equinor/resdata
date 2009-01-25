#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <util.h>
#include <inttypes.h>
#include <list_node.h>
#include <node_data.h>



struct list_node_struct {
  node_data_type * node_data;
  list_node_type *prev_node;
  list_node_type *next_node;
};
  






list_node_type * list_node_get_next(const list_node_type * node) { return node->next_node; }
list_node_type * list_node_get_prev(const list_node_type * node) { return node->prev_node; }

void * list_node_value_ptr(const list_node_type *node) { return (void *) node_data_get_ptr(node->node_data); }


void list_node_link(list_node_type *node1 , list_node_type *node2) {
  if (node1 != NULL) node1->next_node = node2;
  if (node2 != NULL) node2->prev_node = node1;
}


list_node_type * list_node_alloc_managed(const void *value_ptr , int value_size) {
  list_node_type *node;
  node_data_type *node_data = node_data_alloc_buffer(value_ptr , value_size);
  node = list_node_alloc(node_data , NULL , NULL);
  free(node_data);
  return node;
}


list_node_type * list_node_alloc(const void *value , copyc_type *copyc , del_type *del) {
  list_node_type    *node;
  node              = util_malloc(sizeof *node , __func__);
  node->node_data = node_data_alloc_ptr(value , copyc , del); 
  node->prev_node = NULL;
  node->next_node = NULL;
  return node;
}


void list_node_free(list_node_type *node) {
  node_data_free(node->node_data);
  free(node);
}


double list_node_as_double(const list_node_type * node) {
  return node_data_get_double(node->node_data);
}

int list_node_as_int(const list_node_type * node) {
  return node_data_get_int(node->node_data);
}

//const char * list_node_get_string(list_node_type *node) {
//  node_data_type *node_data = list_node_value_ptr(node);
//  return (const char *) node_data_get_data(node_data);
//}
//
//
///*****************************************************************/
//
//#define LIST_NODE_AS_SCALAR(FUNC,TYPE)                         \
//TYPE FUNC(const list_node_type * node) {                       \
//   node_data_type *node_data = list_node_value_ptr(node);      \
//   return *((TYPE *) node_data_get_data(node_data));           \
//} 
//
//
//LIST_NODE_AS_SCALAR(list_node_as_int    , int)
//LIST_NODE_AS_SCALAR(list_node_as_double , double)
//
//
//#undef LIST_NODE_AS_SCALAR
