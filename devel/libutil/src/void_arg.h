#ifndef __VOID_ARG_H__
#define __VOID_ARG_H__
#include <stdio.h>
#include <stdbool.h>
#include <node_ctype.h>

typedef struct void_arg_struct void_arg_type;

void_arg_type * void_arg_alloc(int   , const node_ctype * );
void_arg_type * void_arg_alloc__(int , const node_ctype * , const int * );
void_arg_type * void_arg_alloc1(node_ctype );
void_arg_type * void_arg_alloc2(node_ctype , node_ctype );
void_arg_type * void_arg_alloc3(node_ctype , node_ctype , node_ctype);
void_arg_type * void_arg_alloc4(node_ctype , node_ctype , node_ctype, node_ctype);
void_arg_type * void_arg_alloc5(node_ctype , node_ctype , node_ctype, node_ctype , node_ctype);
void_arg_type * void_arg_alloc6(node_ctype , node_ctype , node_ctype, node_ctype , node_ctype ,node_ctype);
void_arg_type * void_arg_alloc7(node_ctype , node_ctype , node_ctype, node_ctype , node_ctype ,node_ctype , node_ctype);
void_arg_type * void_arg_alloc8(node_ctype , node_ctype , node_ctype, node_ctype , node_ctype ,node_ctype , node_ctype , node_ctype);
void_arg_type * void_arg_alloc9(node_ctype , node_ctype , node_ctype, node_ctype , node_ctype ,node_ctype , node_ctype , node_ctype , node_ctype);
void_arg_type * void_arg_alloc10(node_ctype , node_ctype , node_ctype, node_ctype , node_ctype ,node_ctype , node_ctype , node_ctype , node_ctype , node_ctype);
void            void_arg_free(void_arg_type * );
void            void_arg_free__(void *);
void            void_arg_pack_ptr(void_arg_type * , int , void * );
void *          void_arg_get_ptr(const void_arg_type * , int );
void            void_arg_pack_buffer(void_arg_type * , int , const void * );
void *          void_arg_get_buffer(const void_arg_type * , int );
void            void_arg_fscanf(void_arg_type * , FILE * );

void_arg_type * void_arg_alloc_double(double );
void_arg_type * void_arg_alloc_int(int   );
void_arg_type * void_arg_alloc_ptr(void  *);
void_arg_type * void_arg_alloc_buffer(int , const void *);
void            void_arg_fprintf_typed(const void_arg_type * , FILE * );


#define VOID_ARG_TYPED_PACK_HEADER(type) void void_arg_pack_ ## type (void_arg_type * , int  , type );
#define VOID_ARG_TYPED_GET_HEADER(type)  type void_arg_get_  ## type (const void_arg_type *, int );

VOID_ARG_TYPED_GET_HEADER(int)
VOID_ARG_TYPED_GET_HEADER(char)
VOID_ARG_TYPED_GET_HEADER(float)
VOID_ARG_TYPED_GET_HEADER(double)
VOID_ARG_TYPED_GET_HEADER(size_t)
VOID_ARG_TYPED_GET_HEADER(bool)

VOID_ARG_TYPED_PACK_HEADER(int)
VOID_ARG_TYPED_PACK_HEADER(char)
VOID_ARG_TYPED_PACK_HEADER(float)
VOID_ARG_TYPED_PACK_HEADER(double)
VOID_ARG_TYPED_PACK_HEADER(size_t)
VOID_ARG_TYPED_PACK_HEADER(bool)

#undef VOID_ARG_TYPED_PACK_HEADER
#undef VOID_ARG_TYPED_GET_HEADER
#endif
