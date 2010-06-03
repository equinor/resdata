#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__
#ifdef __cplusplus 
extern "C" {
#endif


#include <stdbool.h>
#include <subst_list.h>

typedef struct template_struct template_type;


template_type * template_alloc( const char * template_file , bool internalize_template, subst_list_type * parent_subst);
void            template_free( template_type * template );
void            template_instansiate( const template_type * template , const char * __target_file , const subst_list_type * arg_list , bool override_symlink);
void            template_add_arg( template_type * template , const char * key , const char * value );

void            template_clear_args( template_type * template );
int             template_add_args_from_string( template_type * template , const char * arg_string);
char          * template_get_args_as_string( template_type * template );
void            template_set_template_file( template_type * template , const char * template_file);
const char    * template_get_template_file( const template_type * template );


#endif
#ifdef __cplusplus 
}
#endif
