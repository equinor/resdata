#ifndef __ARG_PACK_H__
#define __ARG_PACK_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdbool.h>
#include <node_ctype.h>

typedef struct arg_pack_struct arg_pack_type;
typedef void   (arg_node_free_ftype)  (void *);
typedef void * (arg_node_copyc_ftype) (void *);

arg_pack_type * arg_pack_alloc();
arg_pack_type * arg_pack_safe_cast(void * );
void            arg_pack_free(arg_pack_type * );
void            arg_pack_free__(void *);
void            arg_pack_clear(arg_pack_type *);
void            arg_pack_lock(arg_pack_type *);
void            arg_pack_fscanf(arg_pack_type * arg , FILE * stream);
  void            arg_pack_fprintf(const arg_pack_type * , FILE * );

void            arg_pack_append_ptr(arg_pack_type * , void *);
void            arg_pack_append_owned_ptr(arg_pack_type * , void * , arg_node_free_ftype *);
void            arg_pack_append_copy(arg_pack_type * , void * , arg_node_copyc_ftype * , arg_node_free_ftype *);
void          * arg_pack_iget_ptr(const arg_pack_type * , int);


  /*****************************************************************/

#define APPEND_TYPED_HEADER(type) void arg_pack_append_ ## type (arg_pack_type * , type);
#define IGET_TYPED_HEADER(type)   type arg_pack_iget_ ## type( const arg_pack_type * , int );

APPEND_TYPED_HEADER(int)
APPEND_TYPED_HEADER(bool)
APPEND_TYPED_HEADER(char)
APPEND_TYPED_HEADER(float)
APPEND_TYPED_HEADER(double)
APPEND_TYPED_HEADER(size_t)

IGET_TYPED_HEADER(int)
IGET_TYPED_HEADER(bool)
IGET_TYPED_HEADER(char)
IGET_TYPED_HEADER(float)
IGET_TYPED_HEADER(double)
IGET_TYPED_HEADER(size_t)

#undef APPEND_TYPED_HEADER
#undef GET_TYPED_HEADER


#ifdef __cplusplus
}
#endif
#endif
     
