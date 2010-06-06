#ifndef __VECTOR_H__
#define __VECTOR_H__

#ifdef __cplusplus 
extern "C" {
#endif
#include <node_data.h>

typedef void ( vector_func_type ) (void * , void *);
typedef int  ( vector_cmp_ftype)  (const void * , const void *);

typedef struct vector_struct vector_type;



vector_type * vector_alloc_new();
vector_type * vector_alloc_NULL_initialized( int size );

int  	      vector_append_ref( vector_type * , const void *);
int  	      vector_append_owned_ref( vector_type * , const void * , free_ftype * del);
int           vector_append_copy(vector_type * , const void *, copyc_ftype *, free_ftype *);

void 	      vector_iset_ref( vector_type * , int , const void *);
void 	      vector_iset_owned_ref( vector_type * , int , const void * , free_ftype * del);
void          vector_iset_copy(vector_type * , int , const void *, copyc_ftype *, free_ftype *);

void 	      vector_insert_ref( vector_type * , int , const void *);
void 	      vector_insert_owned_ref( vector_type * , int , const void * , free_ftype * del);
void          vector_insert_copy(vector_type * , int , const void *, copyc_ftype *, free_ftype *);
void          vector_insert_buffer(vector_type * vector , int index , const void * buffer, int buffer_size);

void 	      vector_push_ref( vector_type * ,  const void *);
void 	      vector_push_owned_ref( vector_type * ,  const void * , free_ftype * del);
void          vector_push_copy(vector_type * ,  const void *, copyc_ftype *, free_ftype *);


void          vector_clear(vector_type * vector);
void          vector_free(vector_type * ); 
void          vector_free__( void * arg );
void          vector_append_buffer(vector_type * , const void * , int);
void          vector_push_buffer(vector_type * , const void * , int);
int           vector_get_size(const vector_type * );
const void  * vector_iget_const(const vector_type * , int );
void        * vector_iget(const vector_type * , int );
void          vector_idel(vector_type * vector , int index);
void          vector_shrink( vector_type * vector , int new_size );
void        * vector_get_last(const vector_type * );
const void  * vector_get_last_const(const vector_type * );
int           vector_get_size( const vector_type * );
void        * vector_pop(vector_type * );
void          vector_sort(vector_type * vector , vector_cmp_ftype * cmp);
vector_type * vector_alloc_copy(const vector_type * src , bool deep_copy);

void          vector_iset_buffer(vector_type * vector , int index , const void * buffer, int buffer_size);


#ifdef __cplusplus
}
#endif
#endif
