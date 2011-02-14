/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'list_node.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#ifndef __LIST_NODE_H__
#define __LIST_NODE_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <node_data.h>

typedef struct list_node_struct list_node_type;

list_node_type * list_node_alloc_managed(const void * , int );
void             list_node_link(list_node_type * , list_node_type *);
list_node_type * list_node_get_next(const list_node_type * );
list_node_type * list_node_get_prev(const list_node_type * );
list_node_type * list_node_alloc(const void *, copyc_ftype *, free_ftype *);
void           * list_node_value_ptr(const list_node_type *);
void             list_node_free(list_node_type *);
const     char * list_node_get_string(list_node_type *node);

#define LIST_NODE_AS_SCALAR(FUNC , TYPE)  TYPE FUNC(const list_node_type *node)


LIST_NODE_AS_SCALAR(list_node_as_int    , int);
LIST_NODE_AS_SCALAR(list_node_as_double , double);

#undef LIST_NODE_AS_SCALAR


#ifdef __cplusplus
}
#endif
#endif
