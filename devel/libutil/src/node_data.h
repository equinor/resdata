#ifndef __NODE_DATA_H__
#define __NODE_DATA_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef void       * (  copyc_type) (const void *);
typedef void         (  del_type)   (void *);



typedef struct node_data_struct node_data_type;


void                       node_data_free(node_data_type *);
void                       node_data_free_container(node_data_type * );
node_data_type     	 * node_data_alloc_deep_copy(const node_data_type * );
node_data_type     	 * node_data_alloc_shallow_copy(const node_data_type * );
node_data_type           * node_data_alloc_copy(const node_data_type * node , bool deep_copy);
void     		 * node_data_get_ptr(const node_data_type *);
const void     		 * node_data_get_const_ptr(const node_data_type *);
node_data_type		 * node_data_alloc_buffer(const void *, int );
node_data_type 		 * node_data_alloc_ptr(const void * , copyc_type * , del_type *);

node_data_type		 * node_data_alloc_int(int );
int                        node_data_get_int( const node_data_type * );
node_data_type		 * node_data_alloc_double(double );
double                     node_data_get_double( const node_data_type * );
node_data_type		 * node_data_alloc_string(const char *);
char *                     node_data_get_string( const node_data_type * );



#ifdef __cplusplus
}
#endif
#endif
