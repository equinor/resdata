typedef struct <TYPE>_vector_struct <TYPE>_vector_type;



<TYPE>_vector_type * <TYPE>_vector_alloc( int );
<TYPE>               <TYPE>_vector_iget(const <TYPE>_vector_type * , int);
void                 <TYPE>_vector_iset(<TYPE>_vector_type *       , int , <TYPE>);
void                 <TYPE>_vector_append(<TYPE>_vector_type *     , <TYPE>);
void                 <TYPE>_vector_free(<TYPE>_vector_type *);  
void                 <TYPE>_vector_free_data(<TYPE>_vector_type *);  
void                 <TYPE>_vector_reset(<TYPE>_vector_type *);  
