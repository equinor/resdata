#ifndef __VOID_ARG_H__
#define __VOID_ARG_H__

typedef struct void_arg_struct void_arg_type;


void_arg_type * void_arg_alloc(int , const int * );
void_arg_type * void_arg_alloc2(int , int );
void_arg_type * void_arg_alloc3(int , int , int);
void_arg_type * void_arg_alloc4(int , int , int, int);
void            void_arg_free(void_arg_type * );
void            void_arg_pack_ptr(void_arg_type * , int , void * );
void            void_arg_unpack_ptr(void_arg_type * , int , void * );
void *          void_arg_get_ptr(void_arg_type * , int );


#endif
