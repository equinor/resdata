#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <inttypes.h>
#include <list_node.h>




struct list_node_struct {
  const void     *value;
  
  /*
    For the copy constructor and delete
    operator the logic is very simple:
    
    if they are present they are used.
  */
  copyc_type  	 *copyc;
  del_type    	 *del;
  
  list_node_type *next_node;
};
  

list_node_type * list_node_get_next(const list_node_type * node) { return node->next_node; }

void * list_node_value_ptr(const list_node_type *node) { return (void *) node->value; }

void list_node_set_next(list_node_type *node , const list_node_type *next_node) {
  node->next_node = (list_node_type *) next_node;
}


list_node_type * list_node_alloc(const void *value , copyc_type *copyc , del_type *del) {
  list_node_type *node;
  node      = malloc(sizeof *node);
  node->copyc       = copyc;
  node->del         = del;
  if (node->copyc != NULL) 
    node->value = node->copyc(value);
  else
    node->value   = value;
  
  node->next_node = NULL;
  return node;
}


void list_node_free(list_node_type *node) {
  if (node->del != NULL) 
    node->del((void *) node->value);
  free(node);
}



