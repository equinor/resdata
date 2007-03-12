#ifndef __LIST_H__
#define __LIST_H__

#include <list_node.h>

typedef struct list_struct list_type;

list_type     *  list_alloc(void);
/*
  void             list_del_node(list_type * , list_node_type *);
  void             list_add_node(list_type *, list_node_type *);
*/
void             list_free(list_type *);
bool             list_empty(const list_type * list);
/*list_node_type * list_iget(const list_type *, int);*/

list_node_type * list_get_head(const list_type *);
#endif
