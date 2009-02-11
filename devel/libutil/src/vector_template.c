#include <util.h>
#include <<TYPE>_vector.h>

struct <TYPE>_vector_struct {
  int      alloc_size;
  int      size;
  <TYPE> * data;
};



static void <TYPE>_vector_realloc_data__(<TYPE>_vector_type * vector , int new_alloc_size) {
  if (new_alloc_size > 0)
    /* Obseve that theres is _no_ initialization of the new vector content. */
    vector->data = util_realloc(vector->data , new_alloc_size * sizeof * vector->data , __func__);
  else {
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



<TYPE>_vector_type * <TYPE>_vector_alloc(int alloc_size) {
  <TYPE>_vector_type * vector = util_malloc( sizeof * vector , __func__);
  vector->data = NULL;
  vector->size = 0;
  <TYPE>_vector_realloc_data__(vector , alloc_size);
  return vector;
}


<TYPE> <TYPE>_vector_iget(const <TYPE>_vector_type * vector , int index) {
  <TYPE>_vector_assert_index(vector , index);
  return vector->data[index];
}

/**
   Observe that this function will grow the vector if necessary.
*/
void <TYPE>_vector_iset(<TYPE>_vector_type * vector , int index , <TYPE> value) {
  if (vector->size <= index)
    <TYPE>_vector_realloc_data__(vector , 2 * index);
  vector->data[index] = value;
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
