#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <node_data.h>
#include <vector.h>



#define VECTOR_TYPE_ID      551087
#define VECTOR_DEFAULT_SIZE 10


struct vector_struct {
  int    	   __type_id;    /* internal type id - used for run-time cast check. */
  int    	   alloc_size;   /* The number of elements allocated in the data vector - in general > size. */
  int    	   size;         /* THe number of elements the user has added to the vector. */
  node_data_type **data;         /* node_data instances - which again contain user data. */
};



static void vector_resize__(vector_type * vector, int new_alloc_size) {
  int i;
  if (new_alloc_size < vector->alloc_size) {
    /* The vector is shrinking. */
    for (i=new_alloc_size; i < vector->alloc_size; i++)
      node_data_free( vector->data[i] );
  } 
  
  vector->data = util_realloc( vector->data , new_alloc_size * sizeof * vector->data , __func__);  
  /* The new node_data pointers point anywhere - but that really should not be a problem ... */
  vector->alloc_size = new_alloc_size;
}


vector_type * vector_alloc_new() {
  vector_type * vector = util_malloc( sizeof * vector , __func__);
  vector->__type_id    = VECTOR_TYPE_ID; 
  vector->size         = 0;
  vector->alloc_size   = 0;
  vector->data         = NULL;
  vector_resize__(vector , VECTOR_DEFAULT_SIZE);
  return vector;
}


static void vector_append_node(vector_type * vector , node_data_type * node) {
  if (vector->size == vector->alloc_size)
    vector_resize__(vector , 2*(vector->alloc_size + 1));
  
  vector->data[vector->size] = node;
  vector->size++;
}


/**
   Append a user-pointer which comes without both copy constructor and
   destructor, this implies that the calling scope has FULL
   responsabilty for the storage of the data added to the vector.
*/


void vector_append_ref(vector_type * vector , const void * data) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , NULL);
  vector_append_node(vector , node);
}



/**
   Append a user-pointer which the vector instance takes ownership
   of. This means that when the vector is destroyed it calls the
   destructor on the data which has been supplied. The calling scope
   should basically let this object be - the vector has taken control.
*/


void vector_append_owned_ref(vector_type * vector , const void * data , del_type * del) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , del);
  vector_append_node(vector , node);
}


/*
  This function appends a COPY of user object. This implies that the
  calling scope is still responsible for the instance declared and
  used in that scope, whereas the vector takes responsability of
  freeing it's own copy.
*/


void vector_append_copy(vector_type * vector , const void * data , copyc_type * copyc , del_type * del) {
  node_data_type * node = node_data_alloc_ptr( data, copyc , del);
  vector_append_node(vector , node);
}



/**
   A buffer is unstructured storage (i.e. a void *) which is destroyed
   wth free, and copied with malloc + memcpy. The vector takesa copy
   of the buffer which is inserted (and freed on vector destruction).
*/
   

void vector_append_buffer(vector_type * vector , const void * buffer, int buffer_size) {
  node_data_type * node = node_data_alloc_buffer( buffer , buffer_size );
  vector_append_node(vector , node);
}




const void * vector_iget(const vector_type * vector, int index) {
  if ((index >= 0) && (index < vector->size)) {
    const node_data_type * node = vector->data[index];
    return node_data_get_ptr( node );
  } else {
    util_abort("%s: Invald index:%d  Valid range: [0,%d> \n",__func__ , index , vector->size);
    return NULL;
  }
}



/**
   This vector frees all the storage of the vector, including all the
   nodes which have been installed with a destructor.
*/

void vector_free(vector_type * vector) {
  int i;
  for (i = 0; i < vector->size; i++)
    node_data_free(vector->data[i]);  /* User specific destructors are called here. */
  free(vector->data);
  free(vector);
}
