#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__
#include <subst.h>
#include <stdbool.h>

typedef struct template_struct template_type;


template_type * template_alloc( const char * template_file , bool internalize_template);
void            template_free( template_type * template );
void            template_instansiate( template_type * template , const char * __target_file , const subst_list_type * arg_list );
void            template_add_arg( template_type * template , const char * key , const char * value );

#endif
