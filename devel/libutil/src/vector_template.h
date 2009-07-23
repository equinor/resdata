#ifndef __<TYPE>_VECTOR_H__
#define __<TYPE>_VECTOR_H__
#include <stdio.h>

typedef struct <TYPE>_vector_struct <TYPE>_vector_type;


<TYPE>_vector_type * <TYPE>_vector_alloc( int init_size , <TYPE> );
<TYPE>_vector_type * <TYPE>_vector_alloc_copy( const <TYPE>_vector_type * src);
<TYPE>               <TYPE>_vector_iget(const <TYPE>_vector_type * , int);
<TYPE>               <TYPE>_vector_safe_iget(const <TYPE>_vector_type * , int);
<TYPE>               <TYPE>_vector_get_min(const <TYPE>_vector_type * vector);
<TYPE>               <TYPE>_vector_get_max(const <TYPE>_vector_type * vector);
void                 <TYPE>_vector_iset(<TYPE>_vector_type *       , int , <TYPE>);
void                 <TYPE>_vector_append(<TYPE>_vector_type *     , <TYPE>);
void                 <TYPE>_vector_free(<TYPE>_vector_type *);  
void                 <TYPE>_vector_free__(void *);  
void                 <TYPE>_vector_free_data(<TYPE>_vector_type *);  
void                 <TYPE>_vector_reset(<TYPE>_vector_type *);  
int                  <TYPE>_vector_size(const <TYPE>_vector_type * );
<TYPE>               <TYPE>_vector_pop(<TYPE>_vector_type * vector);
<TYPE>               <TYPE>_vector_get_last(const <TYPE>_vector_type * );
<TYPE> *             <TYPE>_vector_get_ptr(const <TYPE>_vector_type * );
const <TYPE> *       <TYPE>_vector_get_const_ptr(const <TYPE>_vector_type * );
void                 <TYPE>_vector_set_many(<TYPE>_vector_type *  , int  , const <TYPE> *  , int );
void                 <TYPE>_vector_append_many(<TYPE>_vector_type * vector , const <TYPE> * data , int length);
void                 <TYPE>_vector_shrink(<TYPE>_vector_type * );
<TYPE>               <TYPE>_vector_sum(const <TYPE>_vector_type * );
<TYPE>               <TYPE>_vector_get_default(const <TYPE>_vector_type * );
void                 <TYPE>_vector_sort(<TYPE>_vector_type * vector);
void                 <TYPE>_vector_permute(<TYPE>_vector_type * vector , const int * perm);
int *                <TYPE>_vector_alloc_sort_perm(const <TYPE>_vector_type * vector);
void                 <TYPE>_vector_fprintf(const <TYPE>_vector_type * vector , FILE * stream , const char * name , const char * fmt);
void 		     <TYPE>_vector_fwrite(const <TYPE>_vector_type * vector , FILE * stream);
<TYPE>_vector_type * <TYPE>_vector_fread_alloc( FILE * stream );

#endif
