#ifndef __SUBST_H__
#define __SUBST_H__

#ifdef __cplusplus 
extern "C" {
#endif
#include <stdio.h>
#include <util.h>
#include <node_data.h>
#include <subst_func.h>

typedef struct          subst_list_struct subst_list_type;
void                    subst_list_update_buffer( const subst_list_type * subst_list , buffer_type * buffer );
void                    subst_list_insert_func(subst_list_type * subst_list , const char * func_name , const char * local_func_name);
void                    subst_list_fprintf(const subst_list_type * , FILE * stream);
void                    subst_list_set_parent( subst_list_type * subst_list , const subst_list_type * parent);
const subst_list_type * subst_list_get_parent( const subst_list_type * subst_list );
subst_list_type       * subst_list_alloc( const void * input_arg );
subst_list_type       * subst_list_alloc_deep_copy(const subst_list_type * );
void                    subst_list_free(subst_list_type *);
void                    subst_list_clear( subst_list_type * subst_list );
void                    subst_list_insert_copy(subst_list_type *  , const char * , const char * , const char * doc_string);
void                    subst_list_insert_ref(subst_list_type *  , const char * , const char * , const char * doc_string);
void                    subst_list_insert_owned_ref(subst_list_type *  , const char * , const char * , const char * doc_string);

void                  	subst_list_filter_file(const subst_list_type * , const char * , const  char * );
void                    subst_list_update_file(const subst_list_type * , const char * );
void                    subst_list_update_string(const subst_list_type * , char ** );
char                  * subst_list_alloc_filtered_string(const subst_list_type * , const char * );
void                    subst_list_filtered_fprintf(const subst_list_type * , const char *  , FILE * );
int                     subst_list_get_size( const subst_list_type *);
const char            * subst_list_iget_value( const subst_list_type * subst_list , int index);
const char            * subst_list_iget_key( const subst_list_type * subst_list , int index);
const char            * subst_list_iget_doc_string( const subst_list_type * subst_list , int index);

#ifdef __cplusplus 
}
#endif
#endif



