/**
   This file implements a very simple vector functionality. The vector
   is charactereized by the following:

    o The content is only fundamental scalar types, like double, int
      or bool. If you want a vector of compositie types use the
      vector_type implementation.

    o The vector will grow as needed. You can safely set any index in
      the vector, and it will automatically grow. However - this might
      lead to 'holes' - these will be filled with the default value.

    o The implementation is terms of a <TYPE>, the following sed
      one-liner will then produce proper source files:
  
      sed -e'/<TYPE>/int/g' vector_template.c > int_vector.c


   Illustration of the interplay of size, alloc_size and default value.


   1. int_vector_type * vector = int_vector_alloc(2 , 77);
   2. int_vector_append(vector , 1);
   3. int_vector_append(vector , 0);
   4. int_vector_append(vector , 12);
   5. int_vector_iset(vector , 6 , 78);
   6. int_vector_set_default( vector , 99 );

   ------------------------------------

    ·-----·-----·      
1.  | 77  | 77  |    		 								      size = 0, alloc_size = 2
    ·-----·-----·

    ·-----·-----·    		 								        
2.  |  1  | 77  |    		 								      size = 1, alloc_size = 2
    ·-----·-----·

    ·-----·-----·    		 								        
3.  |  1  |  0  |    		 								      size = 2, alloc_size = 2
    ·-----·-----·

    ·-----·-----·-----·-----·    								        
4.  |  1  |  0  |  12 | 77  |    								      size = 3, alloc_size = 4                                             
    ·-----·-----·-----·-----·

    ·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·      
5.  |  1  |  0  |  12 |  77 |  77 | 77  |  78 | 77  | 77  |  77 | 77  | 77  | 77  | 77  | 77  | 77  | size = 7, alloc_size = 12, default = 77
    ·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·

    ·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·      
6.  |  1  |  0  |  12 |  77 |  77 | 77  |  78 | 77  | 99  |  99 | 99  | 99  | 99  | 99  | 99  | 99  | size = 7, alloc_size = 12, default = 99
    ·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·
    
       0     1      2    3     4     5     6     7     8     9    10    11    12    13    14   15


    1. In point 4. above - if you ask the vector for it's size you
       will get 3, and int_vector_iget(vector, 3) will fail because
       that is beyound the end of the vector. 

    2. The size of the vector is the index (+1) of the last validly
       set element in the vector. 

    3. In point 5 above we have grown the vector quite a lot to be
       able to write in index 6, as a results there are now many slots
       in the vector which contain the default value - i.e. 77 in this
       case.

    4. In point 6 we change the default value 77 -> 99, then all the
       77 values from position 7 and onwards are changed to 99; the 77
       values in positions 3,4 & 5 are not touched.

*/


#include <util.h>
#include <string.h>
#include <buffer.h>
#include <<TYPE>_vector.h>

static const char * string_type = "<TYPE>";
#define TYPE_VECTOR_ID ((int *) string_type )[0]

struct <TYPE>_vector_struct {
  UTIL_TYPE_ID_DECLARATION;
  int      alloc_size;    /* The alloceted size of data. */
  int      size;          /* The index of the last valid - i.e. actively set - element in the vector. */
  <TYPE>   default_value; /* The data vector is initialized with this value. */
  <TYPE> * data;          /* The actual data. */
  bool     data_owner;    /* Is the vector owner of the the actual storage data? 
                             If this is false the vector can not be resized. */
};


/**
   This datatype is used when allocating a permutation list
   corresponding to a sorted a xxx_vector instance. This permutation
   list can then be used to reorder several xxx_vector instances.
*/

typedef struct {
  int    index;
  <TYPE> value;
} sort_node_type;



UTIL_SAFE_CAST_FUNCTION(<TYPE>_vector , TYPE_VECTOR_ID);


static void <TYPE>_vector_realloc_data__(<TYPE>_vector_type * vector , int new_alloc_size) {
  if (new_alloc_size != vector->alloc_size) {
    if (vector->data_owner) {
      if (new_alloc_size > 0) {
        int i;
        vector->data = util_realloc(vector->data , new_alloc_size * sizeof * vector->data , __func__);
        for (i=vector->alloc_size;  i < new_alloc_size; i++)
          vector->data[i] = vector->default_value;
      } else {
        if (vector->alloc_size > 0) {
          free(vector->data);
          vector->data = NULL;
        }
      }
      vector->alloc_size = new_alloc_size;
    } else
      util_abort("%s: tried to change the storage are for a shared data segment \n",__func__);
  }
}


static void <TYPE>_vector_assert_index(const <TYPE>_vector_type * vector , int index) {
  if ((index < 0) || (index >= vector->size)) 
    util_abort("%s: index:%d invalid. Valid interval: [0,%d>.\n",__func__ , index , vector->size);
}




static <TYPE>_vector_type * <TYPE>_vector_alloc__(int init_size , <TYPE> default_value, <TYPE> * data, int alloc_size , bool data_owner ) {
  <TYPE>_vector_type * vector = util_malloc( sizeof * vector , __func__);
  UTIL_TYPE_ID_INIT( vector , TYPE_VECTOR_ID);
  vector->default_value       = default_value;

  /**
     Not all combinations of (data, alloc_size, data_owner) are valid:

     1. Creating a new vector instance with fresh storage allocation
        from <TYPE>_vector_alloc():

          data       == NULL
          alloc_size == 0
          data_owner == true


     2. Creating a shared wrapper from the <TYPE>_vector_alloc_shared_wrapper():     
     
          data       != NULL
          data_size   > 0
          data_owner == false


     3. Creating a private wrapper which steals the input data from
        <TYPE>_vector_alloc_private_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == true
          
  */
  
  if (data == NULL) {  /* Case 1: */
    vector->data 	      = NULL;
    vector->data_owner        = true;     /* The input values alloc_size and */
    vector->alloc_size 	      = 0;        /* data_owner are not even consulted. */
  } else {             /* Case 2 & 3 */                
    vector->data 	      = data;
    vector->data_owner        = data_owner;
    vector->alloc_size 	      = alloc_size;
  }
  
  vector->size 	     	      = 0;  
  if (init_size > 0)
    <TYPE>_vector_iset( vector , init_size - 1 , default_value );  /* Filling up the init size elements with the default value */
  
  return vector;
}


/**
   The alloc_size argument is just a hint - the vector will grow as
   needed.
*/
   
<TYPE>_vector_type * <TYPE>_vector_alloc(int init_size , <TYPE> default_value) {
  <TYPE>_vector_type * vector = <TYPE>_vector_alloc__( init_size , default_value , NULL , 0 , true );
  return vector;
}


/**
   This function will change the size of the storage area of the
   vector to become @new_alloc_size elements. If @new_alloc_size is
   less than the current size of the vector, the last elements will be
   lost.
*/

void <TYPE>_vector_resize( <TYPE>_vector_type * vector , int new_alloc_size ) {
  <TYPE>_vector_realloc_data__( vector , new_alloc_size );
}



/**
   This function will allocate a shared wrapper around the input
   pointer data. The vector implementation will work transparently
   with the input data, but the data will not be reallocated, and also
   not freed when exiting, this implies that it is safe (memory wise)
   to work with the memory location pointed to by data from somewhere
   else as well.

   Vector wrappers allocated this way can NOT grow, and will fail HARD
   if a resize beyond alloc_size is attempted - check with
   <TYPE>_vector_growable()
*/


<TYPE>_vector_type * <TYPE>_vector_alloc_shared_wrapper(int init_size, <TYPE> default_value , <TYPE> * data , int alloc_size) {
  return <TYPE>_vector_alloc__( init_size , default_value , data , alloc_size , false );
}


/**
   This function will allocate a vector wrapper around the input
   pointer data. The input data will be hijacked by the vector
   implementation, and the vector will continue like a 100% normal
   vector instance, that includes the capability to resize the data
   area, and the data are will also be freed when the vector is freed.

   Observe that it is (in general) NOT safe to continue to use the
   data pointer from an external scope.
*/


<TYPE>_vector_type * <TYPE>_vector_alloc_private_wrapper(int init_size, <TYPE> default_value , <TYPE> * data , int alloc_size) {
  return <TYPE>_vector_alloc__( init_size , default_value , data , alloc_size , false );
}


/**
   This function will copy all the content from the src vector to the
   target vector. If the the current allocation size of the target
   vector is sufficiently large, it will not be touched.

   Observe that also the default value will be copied.
*/


void <TYPE>_vector_memcpy( const <TYPE>_vector_type * src , <TYPE>_vector_type * target) {
  if (target->alloc_size < src->size)
    <TYPE>_vector_realloc_data__( target , src->alloc_size );  /* Must grow the target vector */
  
  /* Copy the content. */
  memcpy(target->data , src->data , src->size * sizeof * src->data );
  
  /* Copy the headers */
  target->size          = src->size;
  target->default_value = src->default_value;
}




<TYPE>_vector_type * <TYPE>_vector_alloc_copy( const <TYPE>_vector_type * src) {
  <TYPE>_vector_type * copy = <TYPE>_vector_alloc( src->size , src->default_value );
  <TYPE>_vector_realloc_data__( copy , src->alloc_size );
  copy->size = src->size;
  memcpy(copy->data , src->data , src->alloc_size * sizeof * src->data );
  return copy;
}

bool <TYPE>_vector_growable( const <TYPE>_vector_type * vector) {
  return vector->data_owner;
}


<TYPE> <TYPE>_vector_get_default(const <TYPE>_vector_type * vector) {
  return vector->default_value;
}


/**
   This will set the default value. This implies that everything
   following the current length of the vector will be set to the new
   default value, whereas values not explicitly set in the interior of
   the vector will retain the old default value.
*/


void <TYPE>_vector_set_default(<TYPE>_vector_type * vector, <TYPE> default_value) {
  vector->default_value = default_value;
  for (int i=vector->size; i < vector->alloc_size; i++)
    vector->data[i] = default_value;
}

/**
   This function will append the value @default_value to the vector,
   and then subsequently set this value as the new default.
*/

void <TYPE>_vector_append_default(<TYPE>_vector_type * vector , <TYPE> default_value) { 
  <TYPE>_vector_append( vector , default_value );
  <TYPE>_vector_set_default( vector , default_value ); 
}


/**
   This function will iset the value @default_value into the vector,
   and then subsequently set this value as the new default. 

   1.    V = [1 , 2 , 3 , 4 ], default = 77


   2.  _iset_default(v , 10 , 66)

         V = [1 ,2 , 3 ,  4 , 77, 77, 77, 77, 77, 77, 66] 

       I.e. before setting the 66 value all values up to @index are
       filled with the current default.

   3. If @index is inside the current data region, there will be no
      effect of the current default, i.e. _iset_default(v , 2 , 66)
      will just give:
 
         V = [1 ,2 , 66 ,  4 ]

      and 66 as the new default value.
*/

void <TYPE>_vector_iset_default(<TYPE>_vector_type * vector , int index , <TYPE> default_value) { 
  <TYPE>_vector_iset( vector , index , default_value );
  <TYPE>_vector_set_default( vector , default_value ); 
}





<TYPE> <TYPE>_vector_iget(const <TYPE>_vector_type * vector , int index) {
  <TYPE>_vector_assert_index(vector , index);
  return vector->data[index];
}

/**
   This might very well operate on a default value. 
*/
void <TYPE>_vector_imul(<TYPE>_vector_type * vector, int index, <TYPE> factor) {
  <TYPE>_vector_assert_index(vector , index);
  vector->data[index] *= factor;
}


void <TYPE>_vector_scale(<TYPE>_vector_type * vector, <TYPE> factor) {
  for (int i=0; i < vector->size; i++)
    vector->data[i] *= factor;
}



/* Will return default value if index > size. Will fail HARD on negative indices (not that safe) ....*/

<TYPE> <TYPE>_vector_safe_iget(const <TYPE>_vector_type * vector, int index) {
  if (index >= vector->size)
    return vector->default_value;
  else {
    if (index >= 0)
      return vector->data[index];
    else {
      util_abort("%s: index:%d is invalid - only accepts positive indices.\n",__func__ , index);
      return -1;
    }
  }
}

/** Will abort is size == 0 */
<TYPE> <TYPE>_vector_get_last(const <TYPE>_vector_type * vector) {
  return <TYPE>_vector_iget(vector , vector->size - 1);
}


/** Will abort is size == 0 */
<TYPE> <TYPE>_vector_get_first(const <TYPE>_vector_type * vector) {
  return <TYPE>_vector_iget(vector , 0);
}



/**
   Observe that this function will grow the vector if necessary. If
   index > size - i.e. leaving holes in the vector, these are
   explicitly set to the default value. If a reallocation is needed it
   is done in the realloc routine, otherwise it is done here.
*/

void <TYPE>_vector_iset(<TYPE>_vector_type * vector , int index , <TYPE> value) {
  if (index < 0) 
    util_abort("%s: Sorry - can NOT set negative indices. called with index:%d \n",__func__ , index);
  {
    if (vector->alloc_size <= index)
      <TYPE>_vector_realloc_data__(vector , 2 * (index + 1));  /* Must have ( + 1) here to ensure we are not doing 2*0 */
    vector->data[index] = value;
    if (index >= vector->size) {
      int i;
      for (i=vector->size; i < index; i++)
        vector->data[i] = vector->default_value;
      vector->size = index + 1;
    }
  }
}

/**
   This function invokes _iset - i.e. growing the vector if needed. If
   the index is not currently set, the default value will be used.
*/

<TYPE> <TYPE>_vector_iadd( <TYPE>_vector_type * vector , int index , <TYPE> delta) {
  <TYPE> new_value     = <TYPE>_vector_safe_iget(vector , index ) + delta;
  <TYPE>_vector_iset( vector , index , new_value );
  return new_value;
}


void <TYPE>_vector_append(<TYPE>_vector_type * vector , <TYPE> value) {
  <TYPE>_vector_iset(vector , vector->size , value);
}


void <TYPE>_vector_reset(<TYPE>_vector_type * vector) {
  vector->size = 0;
}







void <TYPE>_vector_free_data(<TYPE>_vector_type * vector) {
  <TYPE>_vector_reset(vector);
  <TYPE>_vector_realloc_data__(vector , 0);
}


void <TYPE>_vector_free(<TYPE>_vector_type * vector) {
  if (vector->data_owner)
    util_safe_free( vector->data );
  free( vector );
}


void <TYPE>_vector_free__(void * __vector) {
  <TYPE>_vector_type * vector = (<TYPE>_vector_type *) __vector;
  <TYPE>_vector_free( vector );
}


void <TYPE>_vector_reset__(void * __vector) {
  <TYPE>_vector_type * vector = <TYPE>_vector_safe_cast( __vector );
  <TYPE>_vector_reset( vector );
}


int <TYPE>_vector_size(const <TYPE>_vector_type * vector) {
  return vector->size;
}


/**
   The pop function will remove the last element from the vector and
   return it. If the vector is empty - it will abort.
*/

<TYPE> <TYPE>_vector_pop(<TYPE>_vector_type * vector) {
  if (vector->size > 0) {
    <TYPE> value = vector->data[vector->size - 1];
    vector->size--;
    return value;
  } else {
    util_abort("%s: trying to pop from empty vector \n",__func__);
    return -1;   /* Compiler shut up. */
  }
}



<TYPE> * <TYPE>_vector_get_ptr(const <TYPE>_vector_type * vector) {
  return vector->data;
}


const <TYPE> * <TYPE>_vector_get_const_ptr(const <TYPE>_vector_type * vector) {
  return vector->data;
}

/**
   Observe that there is a principle difference between the get_ptr()
   functions and alloc_data_copy() when the vector has zero size. The
   former functions will always return valid (<TYPE> *) pointer,
   altough possibly none of the elements have been set by the vector
   instance, whereas the alloc_data_copy() function will return NULL
   in that case.
*/

<TYPE> * <TYPE>_vector_alloc_data_copy( const <TYPE>_vector_type * vector ) {
  int      size = vector->size * sizeof ( <TYPE> ); 
  <TYPE> * copy = util_malloc(size , __func__);
  if (copy != NULL)
    memcpy( copy , vector->data , size);
  return copy;
}



void <TYPE>_vector_set_many(<TYPE>_vector_type * vector , int index , const <TYPE> * data , int length) {
  int min_size = index + length;
  if (min_size > vector->alloc_size)
    <TYPE>_vector_realloc_data__(vector , 2 * min_size);
  memcpy( &vector->data[index] , data , length * sizeof * data);
  if (min_size > vector->size)
    vector->size = min_size;
}


void <TYPE>_vector_append_many(<TYPE>_vector_type * vector , const <TYPE> * data , int length) {
  <TYPE>_vector_set_many( vector , <TYPE>_vector_size( vector ) , data , length);
}


/**
   This will realloc the vector so that alloc_size exactly matches
   size.
*/
void <TYPE>_vector_shrink(<TYPE>_vector_type * vector) {
  <TYPE>_vector_realloc_data__(vector , vector->size);
}


<TYPE> <TYPE>_vector_get_max(const <TYPE>_vector_type * vector) {
  int i;
  <TYPE> max_value = vector->data[0];
  for (i=0; i < vector->size; i++) {
    if (vector->data[i] > max_value)
      max_value = vector->data[i];
  }
  return max_value;
}

<TYPE> <TYPE>_vector_get_min(const <TYPE>_vector_type * vector) {
  int i;
  <TYPE> min_value = vector->data[0];
  for (i=0; i < vector->size; i++) {
    if (vector->data[i] < min_value)
      min_value = vector->data[i];
  }
  return min_value;
}


<TYPE> <TYPE>_vector_sum(const <TYPE>_vector_type * vector) {
  int i;
  <TYPE> sum = 0;
  for (i=0; i < vector->size; i++)
    sum += vector->data[i];
  return sum;
}





/*****************************************************************/
/* Functions for sorting a vector instance. */

static int <TYPE>_vector_cmp(const void *_a, const void *_b) {
  <TYPE> a = *((<TYPE> *) _a);
  <TYPE> b = *((<TYPE> *) _b);

  if (a > b)
    return 1;
  
  if (a < b)
    return -1;

  return 0;
}

/**
   The input vector will be altered in place, so that the vector only
   contains every numerical value __once__. On exit the values will be
   sorted in increasing order.

   vector = <7 , 0 , 1 , 7 , 1 , 0 , 7 , 1> => <0,1,7>
*/

void <TYPE>_vector_select_unique(<TYPE>_vector_type * vector) {
  <TYPE>_vector_type * copy = <TYPE>_vector_alloc_copy( vector );
  <TYPE>_vector_sort( copy );
  <TYPE>_vector_reset( vector );
  {
    int i;
    <TYPE> previous_value = <TYPE>_vector_iget( copy , 0);
    <TYPE>_vector_append( vector , previous_value);

    for (i=1; i <  copy->size; i++) {
      <TYPE> value = <TYPE>_vector_iget( copy , i );
      if (value != previous_value)
        <TYPE>_vector_append( vector , value);
      previous_value = value;
    }
  }
  <TYPE>_vector_free( copy );
}

/**
   Inplace numerical sort of the vector; sorted in increasing order.
*/
void <TYPE>_vector_sort(<TYPE>_vector_type * vector) {
  qsort(vector->data , vector->size , sizeof * vector->data ,  <TYPE>_vector_cmp);
}



static int <TYPE>_vector_cmp_node(const void *_a, const void *_b) {
  sort_node_type a = *((sort_node_type *) _a);
  sort_node_type b = *((sort_node_type *) _b);

  if (a.value < b.value)
    return -1;
  
  if (a.value > b.value)
    return 1;
  
  return 0;
}


/**
   This function will allocate a (int *) pointer of indices,
   corresponding to the permutations of the elements in the vector to
   get it into sorted order. This permutation can then be used to sort
   several vectors identically:

   int_vector_type    * v1;
   bool_vector_type   * v2;
   double_vector_type * v2;
   .....
   .....

   {
      int * sort_perm = int_vector_alloc_sort_perm( v1 );
      int_vector_permute( v1 , sort_perm );
      bool_vector_permute( v2 , sort_perm );
      double_vector_permute( v3 , sort_perm );
      free(sort_perm);
   }
   
*/



   
int * <TYPE>_vector_alloc_sort_perm(const <TYPE>_vector_type * vector) {
  int * sort_perm             = util_malloc( vector->size * sizeof * sort_perm , __func__);
  sort_node_type * sort_nodes = util_malloc( vector->size * sizeof * sort_nodes , __func__);
  int i;
  for (i=0; i < vector->size; i++) {
    sort_nodes[i].index = i;
    sort_nodes[i].value = vector->data[i];
  }
  qsort(sort_nodes , vector->size , sizeof * sort_nodes ,  <TYPE>_vector_cmp_node);
    
  for (i=0; i < vector->size; i++)
    sort_perm[i] = sort_nodes[i].index;
  
  free( sort_nodes );
  return sort_perm;
}



void <TYPE>_vector_permute(<TYPE>_vector_type * vector , const int * perm) {
  int i;
  <TYPE> * tmp = util_alloc_copy( vector->data , sizeof * tmp * vector->size , __func__);
  for (i=0; i < vector->size; i++) 
    vector->data[i] = tmp[perm[i]];
  free( tmp );
}


/*****************************************************************/


void <TYPE>_vector_fprintf(const <TYPE>_vector_type * vector , FILE * stream , const char * name , const char * fmt) {
  int i;
  if (name != NULL)
    fprintf(stream , "%s = [" , name);
  else
    fprintf(stream , "[");

  for (i = 0; i < vector->size; i++) {
    fprintf(stream , fmt , vector->data[i]);
    if (i < (vector->size - 1))
      fprintf(stream , ", ");
  }

  fprintf(stream , "]\n");
}


/*
  This function does not consider the default value; it does a
  vector_resize based on the input size.
*/
void <TYPE>_vector_fread_data( <TYPE>_vector_type * vector , int size, FILE * stream) {
  <TYPE>_vector_realloc_data__( vector , size );
  util_fread( vector->data , sizeof * vector->data , size , stream , __func__);
  vector->size = size;
}



void <TYPE>_vector_fwrite_data( const <TYPE>_vector_type * vector , FILE * stream ) {
  util_fwrite(  vector->data , sizeof * vector->data , vector->size , stream , __func__);
}



/**
   Writing:
   1. Size 
   2. default value
   3. Values
*/

void <TYPE>_vector_fwrite(const <TYPE>_vector_type * vector , FILE * stream) {
  util_fwrite_int( vector->size , stream );
  util_fwrite( &vector->default_value , sizeof vector->default_value , 1 , stream , __func__);
  <TYPE>_vector_fwrite_data( vector , stream );
}



/*
  Observe that this function will reset the default value. 
*/
void <TYPE>_vector_fread( <TYPE>_vector_type * vector , FILE * stream ) {
  <TYPE>     default_value;
  int size = util_fread_int( stream );
  util_fread( &default_value , sizeof default_value , 1 , stream , __func__);
  {
    <TYPE>_vector_set_default( vector , default_value );
    <TYPE>_vector_fread_data( vector , size , stream );
  }
}



<TYPE>_vector_type * <TYPE>_vector_fread_alloc( FILE * stream ) {
  <TYPE>_vector_type * vector = <TYPE>_vector_alloc( 0,0 );
  <TYPE>_vector_fread( vector , stream );
  return vector;
}



void <TYPE>_vector_buffer_fwrite(const <TYPE>_vector_type * vector , buffer_type * buffer) {
  buffer_fwrite_int( buffer , vector->size );
  buffer_fwrite( buffer , &vector->default_value , sizeof vector->default_value , 1 );
  buffer_fwrite( buffer , vector->data , sizeof * vector->data , vector->size );
}


<TYPE>_vector_type * <TYPE>_vector_buffer_fread_alloc( buffer_type * buffer ) {
  <TYPE>     default_value;
  int size = buffer_fread_int( buffer );
  buffer_fread( buffer , &default_value , sizeof default_value , 1 );
  {
    <TYPE>_vector_type * vector = <TYPE>_vector_alloc( size , default_value );
    buffer_fread( buffer , vector->data , sizeof * vector->data , size );
    return vector;
  }
}


/*****************************************************************/

bool <TYPE>_vector_equal(const <TYPE>_vector_type * vector1 , const <TYPE>_vector_type * vector2) {
  if (vector1->size == vector2->size) {
    if (memcmp(vector1->data , vector2->data , sizeof * vector1->data * vector1->size) == 0)
      return true;
    else
      return false;
  } else
    return false;
}


void  <TYPE>_vector_apply(<TYPE>_vector_type * vector , <TYPE>_ftype * func) {
  int i;
  for (i=0; i < vector->size; i++)
    vector->data[i] = func( vector->data[i] );
}
