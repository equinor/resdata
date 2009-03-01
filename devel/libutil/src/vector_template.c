/**
   This file implements a very simple vector functionality. The vector
   is charactereized by the following:

    o The content is only fundamental scalar types, like double, int
      or bool. If you a vector of compositie types use the vector_type
      implementation.

    o The vector will grow as needed. You can safely set any index in
      the value, and it will automatically grow. However - this might
      lead to 'holes' with undefined values in the vector (see
      illustration below).

    o The implementation is terms of a <TYPE>, the following sed
      one-liner will then produce proper source files:
  
      sed -e'/<TYPE>/int/g' vector_template.c > int_vector.c


   Illustration of the interplay of size and alloc_size.


   1. int_vector_type * vector = int_vector_alloc(2);
   2. int_vector_append(vector , 1);
   3. int_vector_append(vector , 0);
   4. int_vector_append(vector , 12);
   5. int_vector_iset(vector , 6 , 78);

   ------------------------------------

    ·-----·-----·      
1.  |     |     |    		 								      size = 0, alloc_size = 2
    ·-----·-----·

    ·-----·-----·    		 								        
2.  |  1  |     |    		 								      size = 1, alloc_size = 2
    ·-----·-----·

    ·-----·-----·    		 								        
3.  |  1  |  0  |    		 								      size = 2, alloc_size = 2
    ·-----·-----·

    ·-----·-----·-----·-----·    								        
4.  |  1  |  0  |  12 |     |    								      size = 3, alloc_size = 4                                             
    ·-----·-----·-----·-----·

    ·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·      
5.  |  1  |  0  |  12 |  X  |  X  |  X  |  78 |     |     |     |     |     |     |     |     |     | size = 7, alloc_size = 12
    ·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·-----·
    
       0     1      2    3     4     5     6     7     8     9    10    11    12    13    14   15


    Now - the important point with this figure is the following:

    1. In point 4. above - if you ask the vector for it's size you
       will get 3, and int_vector_iget(vector, 3) will fail because
       that is beyound the end of the vector. 

    2. The size of the vector is the index (+1) of the last validly
       set element in the vector. 

    3. In point 5 above we have grown the vector quite a lot to be
       able to write in index 6, as a results there are now many slots
       in the vector which contain uninitialized data (marked with 'X'
       above)'. If you query the vector for element 3, 4 or 5 the
       vector will return the default value (set on allocation). Askin
       for element 7 will abort().
*/


#include <util.h>
#include <string.h>
#include <<TYPE>_vector.h>

struct <TYPE>_vector_struct {
  int      alloc_size;    /* The alloceted size of data. */
  int      size;          /* The index of the last valid - i.e. actively set - element in the vector. */
  <TYPE>   default_value; /* The data vector is initialized with this value. */
  <TYPE> * data;          /* The actual data. */
};



static void <TYPE>_vector_realloc_data__(<TYPE>_vector_type * vector , int new_alloc_size) {
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
}

static void <TYPE>_vector_assert_index(const <TYPE>_vector_type * vector , int index) {
  if ((index < 0) || (index >= vector->size)) 
    util_abort("%s: index:%d invalid. Valid interval: [0,%d> \n",__func__ , index , vector->size);
}


/**
   The alloc_size argument is just a hint - the vector will grow as
   needed.
*/
   
<TYPE>_vector_type * <TYPE>_vector_alloc(int alloc_size , <TYPE> default_value) {
  <TYPE>_vector_type * vector = util_malloc( sizeof * vector , __func__);
  vector->data 	     = NULL;
  vector->size 	     = 0;  
  vector->alloc_size = 0;
  vector->default_value = default_value;
  <TYPE>_vector_realloc_data__(vector , alloc_size);
  return vector;
}



<TYPE> <TYPE>_vector_iget(const <TYPE>_vector_type * vector , int index) {
  <TYPE>_vector_assert_index(vector , index);
  return vector->data[index];
}


/* Will return default value if index > size. */
<TYPE> <TYPE>_vector_safe_iget(const <TYPE>_vector_type * vector, int index) {
  if (index >= vector->size)
    return vector->default_value;
  else
    return vector->data[index];
}

/** Will abort is size == 0 */
<TYPE> <TYPE>_vector_get_last(const <TYPE>_vector_type * vector) {
  return <TYPE>_vector_iget(vector , vector->size - 1);
}




/**
   Observe that this function will grow the vector if necessary. If
   index > size - i.e. leaving holes in the vector, these are
   explicitly set to the default value. If a reallocation is needed it
   is done in the realloc routine, otherwise it is done here.
*/

void <TYPE>_vector_iset(<TYPE>_vector_type * vector , int index , <TYPE> value) {
  if (vector->alloc_size <= index)
    <TYPE>_vector_realloc_data__(vector , 2 * index);
  vector->data[index] = value;
  if (index >= vector->size) {
    int i;
    for (i=vector->size; i < index; i++)
      vector->data[i] = vector->default_value;
    vector->size = index + 1;
  }
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
  util_safe_free( vector->data );
  free( vector );
}


void <TYPE>_vector_free__(void * __vector) {
  <TYPE>_vector_type * vector = (<TYPE>_vector_type *) vector;
  <TYPE>_vector_free( vector );
}



int <TYPE>_vector_size(const <TYPE>_vector_type * vector) {
  return vector->size;
}


<TYPE> * <TYPE>_vector_get_ptr(const <TYPE>_vector_type * vector) {
  return vector->data;
}


const <TYPE> * <TYPE>_vector_get_const_ptr(const <TYPE>_vector_type * vector) {
  return vector->data;
}


void <TYPE>_vector_set_many(<TYPE>_vector_type * vector , int index , const <TYPE> * data , int length) {
  int min_size = index + length;
  if (min_size > vector->alloc_size)
    <TYPE>_vector_realloc_data__(vector , 2 * min_size);
  memcpy( &vector->data[index] , data , length * sizeof * data);
  if (min_size > vector->size)
    vector->size = min_size;
}


/**
   This will realloc the vector so that alloc_size exactly matches
   size.
*/
void <TYPE>_vector_shrink(<TYPE>_vector_type * vector) {
  <TYPE>_vector_realloc_data__(vector , vector->size);
}


<TYPE> <TYPE>_vector_sum(const <TYPE>_vector_type * vector) {
  int i;
  <TYPE> sum = 0;
  for (i=0; i < vector->size; i++)
    sum += vector->data[i];
  return sum;
}
