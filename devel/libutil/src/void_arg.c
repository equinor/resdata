#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <void_arg.h>
#include <stdbool.h>
#include <util.h>
#include <node_ctype.h>


#define VOID_ARG_TYPE_SIGNATURE 7651

struct void_arg_struct {
  int            __type_signature;
  int     	 arg_size;
  int     	 byte_size;
  size_t  	*size_list;
  int  	  	*offset_list;
  char 	  	*argBuffer;
  node_ctype *arg_type;
};

/*****************************************************************/

static int void_arg_sizeof(node_ctype arg_type) {
  int size;
  switch (arg_type) {
  case(void_pointer):
    size = sizeof( void * );
    break;
  case(int_value):
    size = sizeof( int );
    break;
  case(double_value):
    size = sizeof( double );
    break;
  case(float_value):
    size = sizeof( float );
    break;
  case(bool_value):
    size = sizeof( bool );
    break;
  case(char_value):
    size = sizeof( char );
    break;
  case(size_t_value):
    size = sizeof( size_t );
    break;
  default:
    fprintf(stderr,"%s: arg_type:%d not recognized - aborting \n",__func__ , arg_type);
    abort();
  }
  return size;
}

static void __void_arg_assert_cast(const void_arg_type * arg) {
  if (arg->__type_signature != VOID_ARG_TYPE_SIGNATURE) {
    fprintf(stderr,"%s: hmmm - the cast to void_arg_type seemed to fail at runtime - aborting\n",__func__);
    abort();
  }
}

static void __void_arg_assert_index(const void_arg_type * arg , int iarg) {
  if (iarg < 0 || iarg >= arg->arg_size) {
    fprintf(stderr,"%s: void_arg() object allocated with %d arguments - %d invalid argument number - aborting \n",__func__ , arg->arg_size , iarg);
    abort();
  }
}

static void __void_arg_assert_type(const void_arg_type * arg , int iarg , node_ctype arg_type) {
  if (arg_type != arg->arg_type[iarg]) {
    fprintf(stderr,"%s: asked for type:\'%s\'  alloc statement:\'%s\'  - aborting \n" , __func__ , node_ctype_name(arg_type) , node_ctype_name(arg->arg_type[iarg]));
    abort();
  }
}

/*****************************************************************/




void_arg_type * void_arg_alloc__(int arg_size , const node_ctype * arg_type , const int * size_list) {
  int i;
  void_arg_type * arg = malloc(sizeof *arg);
  arg->__type_signature = VOID_ARG_TYPE_SIGNATURE;
  arg->arg_size  = arg_size;
  arg->byte_size = 0;
  arg->size_list   = calloc(arg_size , sizeof *arg->size_list);
  arg->offset_list = calloc(arg_size , sizeof *arg->offset_list);
  arg->arg_type    = calloc(arg_size , sizeof *arg->arg_type);
  for (i=0; i < arg_size; i++) {
    arg->arg_type[i]  = arg_type[i];

    if (arg_type[i] == void_buffer) {
      if (size_list != NULL)
	arg->size_list[i] = size_list[i];
      else {
	fprintf(stderr,"%s: when called arg_type = buffer_value - the buffersize *must* be specified in the corresponding size_list argument - aborting \n",__func__);
	abort();
      }
    } else
      arg->size_list[i] = void_arg_sizeof(arg_type[i]);
    
    arg->byte_size   += arg->size_list[i];
    if (i == 0)
      arg->offset_list[i] = 0;
    else
      arg->offset_list[i] = arg->offset_list[i-1] + arg->size_list[i-1];
  }
    
  arg->argBuffer = malloc(arg->byte_size * sizeof * arg->argBuffer);
  return arg;
}

void_arg_type * void_arg_alloc(int arg_size , const node_ctype * arg_type) {
  return void_arg_alloc__(arg_size , arg_type , NULL);
}

void_arg_type * void_arg_alloc1(node_ctype type1) {
  return void_arg_alloc(1 , (const node_ctype[1]) {type1 });
}


void_arg_type * void_arg_alloc2(node_ctype type1, node_ctype type2) {
  return void_arg_alloc(2 , (const node_ctype[2]) {type1 , type2});
}

void_arg_type * void_arg_alloc3(node_ctype type1, node_ctype type2, node_ctype type3) {
  return void_arg_alloc(3 , (const node_ctype[3]) {type1 , type2, type3});
}

void_arg_type * void_arg_alloc4(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4) {
  return void_arg_alloc(4 , (const node_ctype[4]) {type1 , type2, type3, type4});
}

void_arg_type * void_arg_alloc5(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4, node_ctype type5) {
  return void_arg_alloc(5 , (const node_ctype[5]) {type1 , type2, type3, type4 , type5});
}

void_arg_type * void_arg_alloc6(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4, node_ctype type5, node_ctype type6) {
  return void_arg_alloc(6 , (const node_ctype[6]) {type1 , type2, type3, type4 , type5 , type6});
}

void_arg_type * void_arg_alloc7(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4, node_ctype type5, node_ctype type6, node_ctype type7) {
  return void_arg_alloc(7 , (const node_ctype[7]) {type1 , type2, type3, type4 , type5 , type6 , type7});
}

void_arg_type * void_arg_alloc8(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4, node_ctype type5, node_ctype type6, node_ctype type7, node_ctype type8) {
  return void_arg_alloc(8 , (const node_ctype[8]) {type1 , type2, type3, type4 , type5 , type6 , type7 , type8});
}

void_arg_type * void_arg_alloc9(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4, node_ctype type5, node_ctype type6, node_ctype type7, node_ctype type8 , node_ctype type9) {
  return void_arg_alloc(9 , (const node_ctype[9]) {type1 , type2, type3, type4 , type5 , type6 , type7 , type8, type9});
}

void_arg_type * void_arg_alloc10(node_ctype type1, node_ctype type2, node_ctype type3, node_ctype type4, node_ctype type5, node_ctype type6, node_ctype type7, node_ctype type8 , node_ctype type9, node_ctype type10) {
  return void_arg_alloc(10 , (const node_ctype[10]) {type1 , type2, type3, type4 , type5 , type6 , type7 , type8, type9 , type10});
}


void void_arg_free(void_arg_type * arg) {
  free(arg->argBuffer);
  free(arg->size_list);
  free(arg->arg_type);
  free(arg->offset_list);
  free(arg);
}

void void_arg_free__(void * void_arg) {
  void_arg_free((void_arg_type *) void_arg);
}


void void_arg_pack_ptr(void_arg_type * arg, int iarg , void * input) {
  size_t input_adress = ( size_t ) input;
  __void_arg_assert_index(arg , iarg);
  __void_arg_assert_type(arg , iarg , void_pointer);
  memcpy(&arg->argBuffer[arg->offset_list[iarg]] , &input_adress , arg->size_list[iarg]);
}


void * void_arg_get_ptr(const void_arg_type * arg, int iarg) {
  __void_arg_assert_index(arg , iarg);
  __void_arg_assert_type(arg , iarg , void_pointer);
  return (void *) (*( (size_t *) &arg->argBuffer[arg->offset_list[iarg]]));
}


void void_arg_pack_buffer(void_arg_type * arg, int iarg , const void * input) {
  __void_arg_assert_index(arg , iarg);
  memcpy(&arg->argBuffer[arg->offset_list[iarg]] , input , arg->size_list[iarg]);
}

void * void_arg_get_buffer(const void_arg_type * arg, int iarg) {
  __void_arg_assert_cast(arg);
  __void_arg_assert_index(arg , iarg);
  /* No type assert here - can always call pack/get buffer */
  return &arg->argBuffer[arg->offset_list[iarg]];
}


/*****************************************************************/
/* Typed access functions */


#define VOID_ARG_TYPED_GET(type)                                                  \
type void_arg_get_ ## type (const void_arg_type *arg , int iarg) { 		  \
  type output;                                                     		  \
  __void_arg_assert_cast(arg);                                                    \
  __void_arg_assert_index(arg , iarg);                             		  \
  __void_arg_assert_type(arg , iarg , type ## _value);             		  \
  memcpy(&output , &arg->argBuffer[arg->offset_list[iarg]] , arg->size_list[iarg]); \
  return output;                                                                  \
}


VOID_ARG_TYPED_GET(int)
VOID_ARG_TYPED_GET(char)
VOID_ARG_TYPED_GET(float)
VOID_ARG_TYPED_GET(double)
VOID_ARG_TYPED_GET(size_t)
VOID_ARG_TYPED_GET(bool)

#undef VOID_ARG_TYPED_GET


#define VOID_ARG_TYPED_PACK(type)                                                  \
void void_arg_pack_ ## type (void_arg_type *arg , int iarg , type input) {         \
  __void_arg_assert_index(arg , iarg);                                             \
  __void_arg_assert_type(arg , iarg , type ## _value);                             \
  memcpy(&arg->argBuffer[arg->offset_list[iarg]] , &input , arg->size_list[iarg]); \
}


VOID_ARG_TYPED_PACK(int)
VOID_ARG_TYPED_PACK(char)
VOID_ARG_TYPED_PACK(float)
VOID_ARG_TYPED_PACK(double)
VOID_ARG_TYPED_PACK(size_t)
VOID_ARG_TYPED_PACK(bool)

#undef VOID_ARG_TYPED_PACK

/*****************************************************************/

static const char * void_arg_fmt(const void_arg_type * arg , int iarg) {
  __void_arg_assert_index(arg , iarg);
  switch (arg->arg_type[iarg]) {
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
    fprintf(stderr,"%s: arg_type:%d not recognized for scanning \n",__func__ , arg->arg_type[iarg]);
    abort();
  }
}


void void_arg_fscanf(void_arg_type * arg , FILE * stream) {
  int iarg , scan_count;
  char * fmt = NULL;
  for (iarg = 0; iarg  < arg->arg_size; iarg++)
    fmt = util_strcat_realloc(fmt , void_arg_fmt(arg , iarg));
  
  switch(arg->arg_size) {
  case(0):
    break;
  case(1):
    {
      void *arg0;
      arg0 = void_arg_get_buffer(arg , 0);
      scan_count = fscanf(stream , fmt , arg0);
      break;
    }
  case(2):
    {
      void   *arg0, *arg1;
      arg0 = void_arg_get_buffer(arg , 0);
      arg1 = void_arg_get_buffer(arg , 1);

      scan_count = fscanf(stream , fmt , arg0 , arg1);
      break;
    }
  case(3):
    {
      void   *arg0, *arg1 , *arg2;
      arg0 = void_arg_get_buffer(arg , 0);
      arg1 = void_arg_get_buffer(arg , 1);
      arg2 = void_arg_get_buffer(arg , 2);
      
      scan_count = fscanf(stream , fmt , arg0 , arg1 , arg2);
      break;
    }
  case(4):
    {
      void   *arg0, *arg1 , *arg2 , *arg3;
      arg0 = void_arg_get_buffer(arg , 0);
      arg1 = void_arg_get_buffer(arg , 1);
      arg2 = void_arg_get_buffer(arg , 2);
      arg2 = void_arg_get_buffer(arg , 3);
      
      scan_count = fscanf(stream , fmt , arg0 , arg1 , arg2 , arg3);
      break;
    }
  case(5):
    {
      void   *arg0, *arg1 , *arg2 , *arg3, *arg4;
      arg0 = void_arg_get_buffer(arg , 0);
      arg1 = void_arg_get_buffer(arg , 1);
      arg2 = void_arg_get_buffer(arg , 2);
      arg2 = void_arg_get_buffer(arg , 3);
      arg2 = void_arg_get_buffer(arg , 4);

      scan_count = fscanf(stream , fmt , arg0 , arg1 , arg2 , arg3 , arg4);
      break;
    }

  default:
    fprintf(stderr,"%s: sorry %s not allocated for %d arguments - pathetic ehhh?? \n",__func__ , __func__ , arg->arg_size);
    abort();
  }
  if (scan_count != arg->arg_size) {
    fprintf(stderr,"%s: wanted %d arguments - only found: %d \n", __func__ , arg->arg_size , scan_count);
    abort();
  }
  
  free(fmt);
}

void_arg_type * void_arg_safe_cast(void * __void_arg) {
  void_arg_type * void_arg = (void_arg_type * ) __void_arg;
  __void_arg_assert_cast(void_arg);
  return void_arg;
}


/*****************************************************************/

/* 
   These functions are used to create a void_arg instance which
   a type flag according to the node_ctype type and a value.

   usage:

   void_arg_type *void_double = void_arg_alloc_double(78.78);
   void_arg_type *void_int    = void_arg_alloc_int(13);
   ...
   ...
   type = void_arg->arg_type[0];
   if (type == double_value) {
      double value = void_arg_get_double(void_arg , 0);
      printf("The input was a double with value: %g \n",value);
   } else if (type == int_value) {
      int value = void_arg_get_int(void_arg , 0);
      printf("The input was an int with value: %d \n",value);
   }
*/


void_arg_type * void_arg_alloc_double(double value) {
  void_arg_type *arg = void_arg_alloc1(double_value);
  void_arg_pack_double(arg , 0 , value);
  return arg;
}


void_arg_type * void_arg_alloc_int(int value) {
  void_arg_type *arg = void_arg_alloc1(int_value);
  void_arg_pack_int(arg , 0 , value);
  return arg;
}


void_arg_type * void_arg_alloc_ptr(void *ptr) {
  void_arg_type *arg = void_arg_alloc1(void_pointer);
  void_arg_pack_ptr(arg , 0 , ptr );
  return arg;
}


void_arg_type * void_arg_alloc_buffer(int buffer_size, const void * buffer) {
  void_arg_type *arg = void_arg_alloc__(1 , (const node_ctype[1]) { void_buffer} , (const int[1]) {buffer_size});
  void_arg_pack_buffer(arg , 0 , buffer );
  return arg;
}


/* Null'en her er ganske tilfeldig ...*/

/* void_buffer / void_pointer should be char_buffer / char_pointer ? */

void void_arg_fprintf_typed(const void_arg_type * void_arg , FILE * stream) {
  node_ctype type = void_arg->arg_type[0];
  switch (type) {
  case(void_buffer):
    fprintf(stream , "%s" , (char *) void_arg_get_buffer(void_arg , 0));
    break;
  case(void_pointer):
    fprintf(stream , "%s" , (char *) void_arg_get_ptr(void_arg , 0));
    break;
  case(int_value):
    fprintf(stream , "%d" , void_arg_get_int(void_arg , 0));
    break;
  case(double_value):
    fprintf(stream , "%g" , void_arg_get_double(void_arg , 0));
    break;
  default:
    fprintf(stderr,"%s: sorry type:%d not (yet) implemented - aborting\n" , __func__ , type);
    abort();
  }
}
