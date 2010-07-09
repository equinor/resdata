#ifndef __<TYPE>_VECTOR_H__
#define __<TYPE>_VECTOR_H__
#ifdef __cplusplus 
extern "C" {
#endif
#include <stdio.h>
#include <buffer.h>
#include <util.h>

typedef struct <TYPE>_vector_struct <TYPE>_vector_type;
typedef <TYPE> (<TYPE>_ftype) (<TYPE>);



bool                 <TYPE>_vector_growable( const <TYPE>_vector_type * vector);
void                 <TYPE>_vector_select_unique(<TYPE>_vector_type * vector);
<TYPE>_vector_type * <TYPE>_vector_alloc( int init_size , <TYPE> );
<TYPE>_vector_type * <TYPE>_vector_alloc_private_wrapper(int init_size, <TYPE> default_value , <TYPE> * data , int alloc_size);
<TYPE>_vector_type * <TYPE>_vector_alloc_shared_wrapper(int init_size, <TYPE> default_value , <TYPE> * data , int alloc_size);
<TYPE>_vector_type * <TYPE>_vector_alloc_copy( const <TYPE>_vector_type * src);
void                 <TYPE>_vector_imul(<TYPE>_vector_type * vector, int index, <TYPE> factor);
void                 <TYPE>_vector_scale(<TYPE>_vector_type * vector, <TYPE> factor);
<TYPE>               <TYPE>_vector_iget(const <TYPE>_vector_type * , int);
<TYPE>               <TYPE>_vector_safe_iget(const <TYPE>_vector_type * , int);
<TYPE>               <TYPE>_vector_get_min(const <TYPE>_vector_type * vector);
<TYPE>               <TYPE>_vector_get_max(const <TYPE>_vector_type * vector);
<TYPE>               <TYPE>_vector_iadd( <TYPE>_vector_type * vector , int index , <TYPE> delta);
void                 <TYPE>_vector_iset(<TYPE>_vector_type *       , int , <TYPE>);
void                 <TYPE>_vector_append(<TYPE>_vector_type *     , <TYPE>);
void                 <TYPE>_vector_free(<TYPE>_vector_type *);  
void                 <TYPE>_vector_free__(void *);  
void                 <TYPE>_vector_free_data(<TYPE>_vector_type *);  
void                 <TYPE>_vector_reset(<TYPE>_vector_type *); 
void                 <TYPE>_vector_reset__(void * __vector);
int                  <TYPE>_vector_size(const <TYPE>_vector_type * );
<TYPE>               <TYPE>_vector_pop(<TYPE>_vector_type * vector);
<TYPE>               <TYPE>_vector_get_first(const <TYPE>_vector_type * vector);
<TYPE>               <TYPE>_vector_get_last(const <TYPE>_vector_type * );
<TYPE> *             <TYPE>_vector_get_ptr(const <TYPE>_vector_type * );
<TYPE> *             <TYPE>_vector_alloc_data_copy( const <TYPE>_vector_type * vector );
const <TYPE> *       <TYPE>_vector_get_const_ptr(const <TYPE>_vector_type * );
void                 <TYPE>_vector_set_many(<TYPE>_vector_type *  , int  , const <TYPE> *  , int );
void                 <TYPE>_vector_append_many(<TYPE>_vector_type * vector , const <TYPE> * data , int length);
void                 <TYPE>_vector_shrink(<TYPE>_vector_type * );
<TYPE>               <TYPE>_vector_sum(const <TYPE>_vector_type * );
<TYPE>               <TYPE>_vector_get_default(const <TYPE>_vector_type * );
void                 <TYPE>_vector_set_default(<TYPE>_vector_type * vector, <TYPE> default_value);
void                 <TYPE>_vector_append_default(<TYPE>_vector_type * vector , <TYPE> default_value);
void                 <TYPE>_vector_iset_default(<TYPE>_vector_type * vector , int index , <TYPE> default_value);
void                 <TYPE>_vector_sort(<TYPE>_vector_type * vector);
void                 <TYPE>_vector_permute(<TYPE>_vector_type * vector , const int * perm);
int *                <TYPE>_vector_alloc_sort_perm(const <TYPE>_vector_type * vector);
void                 <TYPE>_vector_fprintf(const <TYPE>_vector_type * vector , FILE * stream , const char * name , const char * fmt);
void 		     <TYPE>_vector_fwrite(const <TYPE>_vector_type * vector , FILE * stream);
<TYPE>_vector_type * <TYPE>_vector_fread_alloc( FILE * stream );
<TYPE>_vector_type * <TYPE>_vector_buffer_fread_alloc( buffer_type * buffer );
void                 <TYPE>_vector_buffer_fwrite(const <TYPE>_vector_type * vector , buffer_type * buffer);
void                 <TYPE>_vector_fread( <TYPE>_vector_type * vector , FILE * stream );
void                 <TYPE>_vector_fwrite_data( const <TYPE>_vector_type * vector , FILE * stream );
void                 <TYPE>_vector_fread_data( <TYPE>_vector_type * vector , int size, FILE * stream);
bool                 <TYPE>_vector_equal(const <TYPE>_vector_type * vector1 , const <TYPE>_vector_type * vector2);
void                 <TYPE>_vector_apply(<TYPE>_vector_type * vector , <TYPE>_ftype *func);

UTIL_SAFE_CAST_HEADER( <TYPE>_vector );

#ifdef __cplusplus 
}
#endif
#endif
