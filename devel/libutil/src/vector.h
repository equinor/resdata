#ifndef __VECTOR_H__
#define __VECTOR_H__

#ifdef __cplusplus 
extern "C" {
#endif


typedef struct vector_struct vector_type;


vector_type * vector_alloc_new();
void 	      vector_append_ref( vector_type * , const void *);
void 	      vector_append_owned_ref( vector_type * , const void * , del_type * del);
void          vector_append_copy(vector_type * , const void *, copyc_type *, del_type *);
void          vector_free(vector_type * ); 
void          vector_append_buffer(vector_type * , const void * , int);
int           vector_get_size(const vector_type * );
const void  * vector_iget(const vector_type * , int );


#ifdef __cplusplus
}
#endif
#endif
