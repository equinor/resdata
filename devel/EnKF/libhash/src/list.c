#include <stdlib.h>
#include <stdio.h>
#include <list_node.h>
#include <list.h>
#include <node_data.h>


struct list_struct {
  int length;
  list_node_type * head;
  list_node_type * tail;
};


typedef struct hash_data_struct {
  int    byte_size;
  void  *data;
} hash_data_type;



/*****************************************************************/

static void list_del_node(list_type *list , list_node_type *del_node) {
  if (del_node == NULL) {
    fprintf(stderr,"%s: tried to delete NULL node - aborting \n",__func__);
    abort();
  }
  {
    list_node_type *node, *p_node;
    p_node = NULL;
    node   = list->head;
    while (node != NULL && node != del_node) {
      p_node = node;
      node   = list_node_get_next(node);
    }

    if (node == del_node) {
      if (p_node == NULL) 
	/* 
	   We are attempting to delete the first element in the list.
	*/
	list->head = list_node_get_next(del_node);
      else if (del_node == list->tail) 
	/*
	  We are attempting to delete the last element in the list.
	*/
	list->tail = p_node;
      
      list_node_set_next(p_node , list_node_get_next(del_node));
      list_node_free(del_node);
      list->length--;
    } else {
      fprintf(stderr,"%s: tried to delete node not in list - aborting \n",__func__);
      abort();
    }
  }
}



static void list_append_node(list_type *list , list_node_type *new_node) {
  if (list->head == NULL) 
    list->head = new_node;
  else 
    list_node_set_next(list->tail , new_node);
  list->tail = new_node;
  list->length++;
}



static list_node_type * list_iget_node(const list_type *list, int index) {
  list_node_type *node = list->head;
  int n = 0;
  
  while ((node != NULL) && (n < index))
    node = list_node_get_next(node);
  
  return node;
}


static void list_append_managed_copy(list_type *list , const void *value_ptr , int value_size) {
  list_node_type *node;
  node_data_type list_data;
  list_data.data      = (void *) value_ptr;
  list_data.byte_size = value_size;
  node = list_node_alloc(&list_data , node_data_copyc , node_data_free);
  list_append_node(list , node);
}



/*****************************************************************/
/* Functions which are exported follow below here. */
/*****************************************************************/


void list_free(list_type *list) {
  if (list->head != NULL) {
    list_node_type *node , *next_node;
    node = list->head;
    while (node != NULL) {
      next_node = list_node_get_next(node);
      list_node_free(node);
      node = next_node;
    }
  }
  free(list);
}


list_type * list_alloc(void) {
  list_type * list;
  list = malloc(sizeof *list);
  list->length = 0;
  list->head   = NULL;
  list->tail   = NULL;
  return list;
}



void list_insert_ref(list_type *list , const void *value) {
  list_node_type *node = list_node_alloc(value , NULL , NULL);
  list_append_node(list , node);
}


void list_insert_copy(list_type *list , const void *value , copyc_type *copyc , del_type *del) {
  list_node_type *node = list_node_alloc(value , copyc , del);
  list_append_node(list , node);
}




