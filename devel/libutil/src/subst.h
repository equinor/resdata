#ifndef __SUBST_H__
#define __SUBST_H__

#ifdef __cplusplus 
extern "C" {
#endif
#include <stdio.h>
#include <util.h>
#include <node_data.h>
#include <subst_func.h>

typedef struct subst_list_struct subst_list_type;
void                 subst_list_update_buffer( const subst_list_type * subst_list , buffer_type * buffer );

void                 subst_list_fprintf(const subst_list_type * , FILE * stream);
subst_list_type    * subst_list_alloc( subst_func_pool_type * func_pool);
subst_list_type    * subst_list_alloc_deep_copy(const subst_list_type * );
void                 subst_list_free(subst_list_type *);
void                 subst_list_insert_copy(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_ref(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_owned_ref(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_callback(subst_list_type * subst_list , const char * key , subst_callback_ftype * callback , void * callback_arg , free_ftype * free_callback_arg);

void     	subst_list_filter_file(const subst_list_type * , const char * , const  char * );
void     	subst_list_update_file(const subst_list_type * , const char * );
void            subst_list_update_string(const subst_list_type * , char ** );
char          * subst_list_alloc_filtered_string(const subst_list_type * , const char * );
void            subst_list_filtered_fprintf(const subst_list_type * , const char *  , FILE * );
int             subst_list_get_size( const subst_list_type *);
const char    * subst_list_iget_value( const subst_list_type * subst_list , int index);
const char    * subst_list_iget_key( const subst_list_type * subst_list , int index);

#ifdef __cplusplus 
}
#endif
#endif



