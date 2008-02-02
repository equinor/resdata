#ifndef __VOID_ARG_H__
#define __VOID_ARG_H__
#include <stdio.h>
#include <stdbool.h>

typedef struct void_arg_struct void_arg_type;
typedef enum  {buffer_value = 0 , pointer_value = 1, int_value = 2, double_value = 3, float_value = 4 , char_value = 5 , bool_value = 6 , size_t_value = 7} void_arg_enum;

void_arg_type * void_arg_alloc(int   , const void_arg_enum * );
void_arg_type * void_arg_alloc__(int , const void_arg_enum * , const int * );
void_arg_type * void_arg_alloc1(void_arg_enum );
void_arg_type * void_arg_alloc2(void_arg_enum , void_arg_enum );
void_arg_type * void_arg_alloc3(void_arg_enum , void_arg_enum , void_arg_enum);
void_arg_type * void_arg_alloc4(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum);
void_arg_type * void_arg_alloc5(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum , void_arg_enum);
void_arg_type * void_arg_alloc6(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum , void_arg_enum ,void_arg_enum);
void_arg_type * void_arg_alloc7(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum , void_arg_enum ,void_arg_enum , void_arg_enum);
void_arg_type * void_arg_alloc8(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum , void_arg_enum ,void_arg_enum , void_arg_enum , void_arg_enum);
void_arg_type * void_arg_alloc9(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum , void_arg_enum ,void_arg_enum , void_arg_enum , void_arg_enum , void_arg_enum);
void_arg_type * void_arg_alloc10(void_arg_enum , void_arg_enum , void_arg_enum, void_arg_enum , void_arg_enum ,void_arg_enum , void_arg_enum , void_arg_enum , void_arg_enum , void_arg_enum);
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
