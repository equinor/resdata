#ifndef __LIST_NODE_H__
#define __LIST_NODE_H__
#include <stdbool.h>

typedef struct list_node_struct list_node_type;
typedef const void * (  copyc_type) (const void *);
typedef void         (  del_type)   (void *);

list_node_type * list_node_get_next(const list_node_type * );
void             list_node_set_next(list_node_type * , const list_node_type * );
list_node_type * list_node_alloc(const void *, copyc_type *, del_type *);
void           * list_node_value_ptr(const list_node_type *);
void             list_node_free(list_node_type *);
#endif
