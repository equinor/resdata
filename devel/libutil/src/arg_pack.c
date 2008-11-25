#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arg_pack.h>
#include <stdbool.h>
#include <util.h>
#include <node_ctype.h>


/**
   This file implements a arg_pack structure which is a small
   convienence utility to pack several arguments into one
   argument. The generic use situtation is when calling functions like
   e.g. pthread_create() which take one (void *) as argument. You can
   then pack several arguments into one arg_pack instance, and then
   unpack them at the other end.

   The content of the arg_pack is inserted by appending - there is no
   possibility to set a specified index to a value. When you take them
   out again that is done with indexed get.

   When elements are inserted into the arg_pack, they are inserted
   with a (limited) type information (implictly given by the function
   invoked to insert the argument), and the corresponding typed get
   must be used to unpack the argument again afterwards. The
   excepetion is with the function arg_pack_iget_adress() which can be
   used to extract the referenc of a scalar.



   Example:
   --------

   void some_function(const char * arg1 , int arg2 , double arg3) {
      .....
   }  

   
   void some_function__(void * __arg_pack) {
      arg_pack_type * arg_pack = arg_pack_safe_cast( __arg_pack );
      const char * arg1 = arg_pack_iget_ptr( arg_pack , 0);
      int          arg2 = arg_pack_iget_int( arg_pack , 1);
      double       arg3 = arg_pack_iget_double(arg_pack , 2); 

      some_function( arg1 , arg2 , arg3 );
   }


   .....
   arg_pack_type * arg_pack = arg_pack_alloc();
   arg_pack_append_ptr(arg_pack , "ARG1"); 
   arg_pack_append_int(arg_pack , 1);
   arg_pack_append_double(arg_pack , 3.14159265);

   pthread_create( ,  , some_function__ , arg_pack);

*/

  


#define VOID_ARG_TYPE_SIGNATURE 7651


typedef struct {
  void 	      	       * buffer;        /* This is the actual content - can either point to a remote object, or to storage managed by the arg_pack instance. */
  node_ctype  	         ctype;         /* The type of the data which is stored. */
  arg_node_free_ftype  * destructor;    /* destructor called on buffer - can be NULL. */
  arg_node_copyc_ftype * copyc;         /* copy constructor - will typically be NULL. */
} arg_node_type;



struct arg_pack_struct {
  int             __type_signature;    /* Used to to check run-time casts. */ 
  int             size;                /* The number of arguments appended to this arg_pack instance. */     
  int             alloc_size;          /* The number of nodes allocated to this arg_pack - will in general be greater than size. */
  bool            locked;              /* To insure against unwaranted modifictaions - you can explicitly lock the arg_pack instance. */ 
  arg_node_type **nodes;               /* Vector of nodes */ 
};


/*****************************************************************/
/* First comes the arg_node functions. These are all fully static.*/

static arg_node_type * arg_node_alloc_empty() {
  arg_node_type * node = util_malloc( sizeof * node , __func__);
  node->buffer      = NULL;
  node->destructor  = NULL;
  node->ctype       = invalid_ctype;
  return node;
}


static void arg_node_realloc_buffer(arg_node_type * node , int new_size) {
  node->buffer      = util_realloc(node->buffer , new_size , __func__);
}


static void __arg_node_assert_type(const arg_node_type * node , node_ctype arg_type) {
  if (arg_type != node->ctype) 
    util_abort("%s: asked for type:\'%s\'  inserted as:\'%s\'  - aborting \n" , __func__ , node_ctype_name(arg_type) , node_ctype_name(node->ctype));
}



/*****************************************************************/
/* GET functions. */
#define GET_TYPED(type)\
static type arg_node_get_ ## type(const arg_node_type * node) {\
  __arg_node_assert_type(node , type ## _value);         \
  if (node->ctype == type ## _value) {                   \
     type value;                                         \
     memcpy(&value , node->buffer , sizeof value);       \
     return value;                                       \
   } else {                                              \
     util_abort("%s: type mismatch \n",__func__);        \
     return 0;                                           \
   }                                                     \
}


GET_TYPED(int);
GET_TYPED(float)
GET_TYPED(double)
GET_TYPED(char)
GET_TYPED(bool)
GET_TYPED(size_t);
#undef GET_TYPED

/**
   If the argument is inserted as a pointer, you must use get_ptr ==
   true, otherwise you must use get_ptr == false, and this will give
   you the adress of the scalar.

   Observe that if you call XX_get_ptr() on a pointer which is still
   owned by the arg_pack, you must be careful when freeing the
   arg_pack, as that will delete the pointer you are using as well.
*/

  
static void * arg_node_get_ptr(arg_node_type * node , bool get_ptr) {
  if (get_ptr) {
    if (node->ctype != void_pointer)
      util_abort("%s: tried to get pointer from something not a pointer\n",__func__);
  } else {
    if (node->ctype == void_pointer)
      util_abort("%s: tried to get adress to something already a ponter\n",__func__);
  }
  return node->buffer;
}

/*****************************************************************/
/* SET functions. */

#define SET_TYPED(type)                                                \
static void arg_node_set_ ## type (arg_node_type *node , type value) { \
  arg_node_realloc_buffer(node , sizeof value);                        \
  memcpy(node->buffer , &value , sizeof value);                        \
  node->destructor = NULL;                                             \
  node->ctype      = type ## _value;                                   \
}

SET_TYPED(int);
SET_TYPED(float)
SET_TYPED(double)
SET_TYPED(char)
SET_TYPED(bool)
SET_TYPED(size_t);
#undef SET_TYPED


static void arg_node_set_ptr(arg_node_type * node , void * ptr , arg_node_copyc_ftype * copyc , arg_node_free_ftype * destructor) {
  node->ctype      = void_pointer;
  node->destructor = destructor;
  node->copyc      = copyc; 
  if (copyc != NULL)
    node->buffer = copyc( ptr );
  else
    node->buffer = ptr;
}



/*****************************************************************/


static void arg_node_clear(arg_node_type * node) {
  if (node->ctype == void_pointer) {
    if (node->destructor != NULL) 
      node->destructor( node->buffer );
    /* When you have cleared - must not reuse the thing. */
    node->destructor = NULL;
    node->buffer     = NULL;
    node->copyc      = NULL;
  }
}


static void arg_node_free(arg_node_type * node) {
  arg_node_clear(node);
  util_safe_free(node->buffer);
  free(node);
}



static const char * arg_node_fmt(const arg_node_type *node) {
  switch (node->ctype) {
  case(int_value):
    return " %d";
      break;
  case(double_value):
    return " %lg";
    break;
  case(float_value):
    return " %g";
    break;
  case(bool_value):
    return " %d";
    break;
  case(char_value):
    return " %d";
    break;
  case(size_t_value):
    return " %d";
    break;
  default:
    util_abort("%s: arg_type:%d not recognized for scanning \n",__func__ , node->ctype);
    return ""; /* Dummy to shut up compiler */
  }
}


static void arg_node_fprintf(const arg_node_type * node , FILE * stream) {
  switch (node->ctype) {
  case(int_value):
    fprintf(stream , "int:%d",arg_node_get_int(node));
    break;
  case(double_value):
    fprintf(stream , "double:%g",arg_node_get_double(node));
    break;
  case(void_pointer):
    fprintf(stream , "pointer:<...>");
    break;
  default:
    util_abort("%s: - not implemented for type:%d \n",__func__ , node->ctype);
  }
}


/*****************************************************************/
/* Ending node node functions - starting on functons for the whole pack. */
/*****************************************************************/


static void __arg_pack_assert_cast(const arg_pack_type * arg) {
  if (arg == NULL) 
    util_abort("%s: arrived with arg = NULL - this is broken !! \n",__func__);
  
  if (arg->__type_signature != VOID_ARG_TYPE_SIGNATURE) 
    util_abort("%s: hmmm - the cast to arg_pack_type seemed to fail at runtime - aborting\n",__func__);
}


static void __arg_pack_assert_index(const arg_pack_type * arg , int iarg) {
  if (iarg < 0 || iarg >= arg->size) 
    util_abort("%s: arg_pack() object allocated with %d arguments - %d invalid argument number - aborting \n",__func__ , arg->size , iarg);
}


static void arg_pack_realloc_nodes(arg_pack_type * arg_pack , int new_size) {
  arg_pack->nodes      = util_realloc(arg_pack->nodes , new_size * sizeof * arg_pack->nodes , __func__);
  {
    int i;
    for (i = arg_pack->alloc_size; i < new_size; i++)
      arg_pack->nodes[i] = arg_node_alloc_empty();
  }
  arg_pack->alloc_size = new_size;
}


static arg_node_type * arg_pack_get_append_node(arg_pack_type * arg_pack) {
  if (arg_pack->locked) {
    util_abort("%s: tryng to append to a locked arg_pack instance \n",__func__);
    return NULL;
  }
  {
    if (arg_pack->size == arg_pack->alloc_size)
      arg_pack_realloc_nodes(arg_pack , 1 + arg_pack->alloc_size * 2);
    return arg_pack->nodes[arg_pack->size];
  }
}


arg_pack_type * arg_pack_safe_cast(void * __arg_pack) {
  arg_pack_type * arg_pack = (arg_pack_type * ) __arg_pack;
  __arg_pack_assert_cast(arg_pack);
  return arg_pack;
}



void arg_pack_lock(arg_pack_type * arg_pack) {
  arg_pack->locked = true;
}



arg_pack_type * arg_pack_alloc() {
  arg_pack_type * arg_pack = util_malloc(sizeof * arg_pack , __func__);
  arg_pack->__type_signature = VOID_ARG_TYPE_SIGNATURE;
  arg_pack->nodes      = NULL;
  arg_pack->alloc_size = 0;
  arg_pack->locked     = false;
  arg_pack_realloc_nodes(arg_pack , 4);
  arg_pack_clear(arg_pack);
  return arg_pack;
}


void arg_pack_free(arg_pack_type * arg_pack) {
  int i;

  for (i=0; i < arg_pack->alloc_size; i++) 
    arg_node_free( arg_pack->nodes[i] );

  free(arg_pack->nodes);
  free(arg_pack);
}


void arg_pack_free__(void * __arg_pack) {
  arg_pack_type * arg_pack = arg_pack_safe_cast( __arg_pack );
  arg_pack_free( arg_pack );
}



void arg_pack_clear(arg_pack_type * arg_pack) {
  if (arg_pack->locked) 
    util_abort("%s: arg_pack has been locked - abortng \n",__func__);
  {
    int i;
    for ( i=0; i < arg_pack->size; i++)
      arg_node_clear(arg_pack->nodes[i]);
    arg_pack->size = 0;
  }
}


/******************************************************************/
/* Access functions:

  1. Append
  2. iget 


******************************************************************/

#define APPEND_TYPED(type)                                         \
void arg_pack_append_ ## type (arg_pack_type *pack , type value) { \
  arg_node_type * node = arg_pack_get_append_node( pack );            \
  arg_node_set_ ## type(node , value);                             \
  pack->size++;                                                    \
}


#define IGET_TYPED(type)\
type arg_pack_iget_ ## type(const arg_pack_type * pack, int index) { \
  __arg_pack_assert_index( pack , index);                      \
  {                                                            \
    arg_node_type * node = pack->nodes[index];                 \
    return arg_node_get_ ## type ( node );                     \
  }                                                            \
}


APPEND_TYPED(int);
APPEND_TYPED(bool);
APPEND_TYPED(float);
APPEND_TYPED(double);
APPEND_TYPED(char);
APPEND_TYPED(size_t);

IGET_TYPED(int);
IGET_TYPED(bool);
IGET_TYPED(float);
IGET_TYPED(double);
IGET_TYPED(char);
IGET_TYPED(size_t);

#undef APPEND_TYPED
#undef IGET_TYPED



void * arg_pack_iget_ptr(const arg_pack_type * arg , int iarg) {
  __arg_pack_assert_index(arg , iarg);
  return arg_node_get_ptr(arg->nodes[iarg] , true);
}


void * arg_pack_iget_address(const arg_pack_type * arg , int iarg) {
  __arg_pack_assert_index(arg , iarg);
  return arg_node_get_ptr(arg->nodes[iarg] , false);
}


void  arg_pack_append_copy(arg_pack_type * arg_pack , void * ptr, arg_node_copyc_ftype * copyc , arg_node_free_ftype * freef) {
  arg_node_type * node = arg_pack_get_append_node( arg_pack );          
  arg_node_set_ptr(node , ptr , copyc , freef);
  arg_pack->size++;
}


void arg_pack_append_ptr(arg_pack_type * arg_pack, void * ptr) {
  arg_pack_append_copy(arg_pack , ptr , NULL , NULL);
}

void arg_pack_append_owned_ptr(arg_pack_type * arg_pack, void * ptr, arg_node_free_ftype * freef) {
  arg_pack_append_copy(arg_pack , ptr , NULL , freef );
}


/******************************************************************/
/* Functions for formatted reading/writing of arg_pack instances. */



void arg_pack_fscanf(arg_pack_type * arg , FILE * stream) {
  int scan_count = 0;
  int iarg;
  char * fmt = NULL;
  for (iarg = 0; iarg  < arg->size; iarg++) {
    arg_node_type * node = arg->nodes[iarg];
    fmt = util_strcat_realloc(fmt , arg_node_fmt(node));
  }
  
  switch(arg->size) {
  case(0):
    break;
  case(1):
    {
      void *arg0;
      arg0 = arg_pack_iget_address(arg , 0);
      scan_count = fscanf(stream , fmt , arg0);
      break;
    }
  case(2):
    {
      void   *arg0, *arg1;
      arg0 = arg_pack_iget_address(arg , 0);
      arg1 = arg_pack_iget_address(arg , 1);

      scan_count = fscanf(stream , fmt , arg0 , arg1);
      break;
    }
  case(3):
    {
      void   *arg0, *arg1 , *arg2;
      arg0 = arg_pack_iget_address(arg , 0);
      arg1 = arg_pack_iget_address(arg , 1);
      arg2 = arg_pack_iget_address(arg , 2);
      
      scan_count = fscanf(stream , fmt , arg0 , arg1 , arg2);
      break;
    }
  case(4):
    {
      void   *arg0, *arg1 , *arg2 , *arg3;
      arg0 = arg_pack_iget_address(arg , 0);
      arg1 = arg_pack_iget_address(arg , 1);
      arg2 = arg_pack_iget_address(arg , 2);
      arg3 = arg_pack_iget_address(arg , 3);
      
      scan_count = fscanf(stream , fmt , arg0 , arg1 , arg2 , arg3);
      break;
    }
  case(5):
    {
      void   *arg0, *arg1 , *arg2 , *arg3, *arg4;
      arg0 = arg_pack_iget_address(arg , 0);
      arg1 = arg_pack_iget_address(arg , 1);
      arg2 = arg_pack_iget_address(arg , 2);
      arg3 = arg_pack_iget_address(arg , 3);
      arg4 = arg_pack_iget_address(arg , 4);

      scan_count = fscanf(stream , fmt , arg0 , arg1 , arg2 , arg3 , arg4);
      break;
    }

  default:
    util_abort("%s: sorry %s not allocated for %d arguments - pathetic ehhh?? \n",__func__ , __func__ , arg->size);
  }
  
  if (scan_count != arg->size) 
    util_abort("%s: wanted %d arguments - only found: %d \n", __func__ , arg->size , scan_count);
    
  free(fmt);
}


void arg_pack_fprintf(const arg_pack_type * arg_pack , FILE * stream) {
  int iarg;
  fprintf(stream," [");
  for (iarg = 0; iarg  < arg_pack->size; iarg++) {
    arg_node_type * node = arg_pack->nodes[iarg];
    arg_node_fprintf(node , stream);
    if (iarg < (arg_pack->size - 1))
      fprintf(stream,", ");
  }
  fprintf(stream, "]\n");
}





/*****************************************************************/

/* 
   These functions are used to create a arg_pack instance which
   a type flag according to the node_ctype type and a value.

   usage:

   arg_pack_type *void_double = arg_pack_alloc_double(78.78);
   arg_pack_type *void_int    = arg_pack_alloc_int(13);
   ...
   ...
   type = arg_pack->arg_type[0];
   if (type == double_value) {
      double value = arg_pack_get_double(arg_pack , 0);
      printf("The input was a double with value: %g \n",value);
   } else if (type == int_value) {
      int value = arg_pack_get_int(arg_pack , 0);
      printf("The input was an int with value: %d \n",value);
   }
*/


/*arg_pack_type * arg_pack_alloc_double(double value) {
  arg_pack_type *arg = arg_pack_alloc1(double_value);
  arg_pack_pack_double(arg , 0 , value);
  return arg;
}


arg_pack_type * arg_pack_alloc_int(int value) {
  arg_pack_type *arg = arg_pack_alloc1(int_value);
  arg_pack_pack_int(arg , 0 , value);
  return arg;
}


arg_pack_type * arg_pack_alloc_ptr(void *ptr) {
  arg_pack_type *arg = arg_pack_alloc1(void_pointer);
  arg_pack_pack_ptr(arg , 0 , ptr );
  return arg;
}


arg_pack_type * arg_pack_alloc_buffer(int buffer_size, const void * buffer) {
  arg_pack_type *arg = arg_pack_alloc__(1 , (const node_ctype[1]) { void_buffer} , (const int[1]) {buffer_size});
  arg_pack_pack_buffer(arg , 0 , buffer );
  return arg;
}


arg_pack_type * arg_pack_alloc_string(const char * s) {
  return arg_pack_alloc_buffer(strlen(s) + 1 , s);
}

*/
