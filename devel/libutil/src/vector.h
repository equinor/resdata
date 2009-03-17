#ifndef __VECTOR_H__
#define __VECTOR_H__

#ifdef __cplusplus 
extern "C" {
#endif


typedef void ( vector_func_type ) (void * , void *);

typedef struct vector_struct vector_type;



vector_type * vector_alloc_new();
int  	      vector_append_ref( vector_type * , const void *);
int  	      vector_append_owned_ref( vector_type * , const void * , del_type * del);
int           vector_append_copy(vector_type * , const void *, copyc_type *, del_type *);
void 	      vector_insert_ref( vector_type * , int , const void *);
void 	      vector_insert_owned_ref( vector_type * , int , const void * , del_type * del);
void          vector_insert_copy(vector_type * , int , const void *, copyc_type *, del_type *);
void          vector_free(vector_type * ); 
void          vector_append_buffer(vector_type * , const void * , int);
int           vector_get_size(const vector_type * );
const void  * vector_iget_const(const vector_type * , int );
void        * vector_iget(const vector_type * , int );
void        * vector_get_last(const vector_type * );
const void  * vector_get_last_const(const vector_type * );
int           vector_get_size( const vector_type * );
void        * vector_pop(vector_type * );

#ifdef __cplusplus
}
#endif
#endif
