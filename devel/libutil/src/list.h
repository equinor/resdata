/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'list.h' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#ifndef __LIST_H__
#define __LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <list_node.h>

typedef struct list_struct list_type;

list_type      * list_alloc(void);
void             list_del_node(list_type *list , list_node_type *);
/*
  void             list_add_node(list_type *, list_node_type *);
*/
void             list_free(list_type *);
list_node_type * list_iget_node(const list_type *, int );
void           * list_iget_node_value_ptr(const list_type *, int );
list_node_type * list_get_head(const list_type *);
list_node_type * list_get_tail(const list_type *);
list_node_type * list_append_ref(list_type *list , const void *);
list_node_type * list_append_list_owned_ref(list_type *, const void *, free_ftype *);
int              list_get_size(const list_type *);
list_node_type * list_append_string_copy(list_type *, const char * );
list_node_type * list_append_copy(list_type *, const void *, copyc_ftype *, free_ftype *);
#ifdef __cplusplus
}
#endif
#endif
