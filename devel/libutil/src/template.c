#include <stdlib.h>
#include <stdbool.h>
#include <subst_list.h>
#include <subst_func.h>
#include <template.h>



#define TEMPLATE_TYPE_ID 7781045

struct template_struct {
  UTIL_TYPE_ID_DECLARATION;
  char            * template_file;           /* The template file - if internalize_template == false this filename can contain keys which will be replaced at instantiation time. */
  char            * template_buffer;         /* The content of the template buffer; only has valid content if internalize_template == true. */
  bool              internalize_template;    /* Should the template be loadad and internalized at template_alloc(). */
  subst_list_type * arg_list;                /* Key-value mapping established at alloc time. */
};




/**
   Iff the template is set up with internaliz_template == false the
   template content is loaded at instantiation time, and in that case
   the name of the template file can contain substitution characters -
   i.e. in this case different instance can use different source
   templates.

   To avoid race issues this function does not set actually update the
   state of the template object.
*/

static char * template_load( const template_type * template , const subst_list_type * ext_arg_list) {
  int buffer_size;
  char * template_file = util_alloc_string_copy( template->template_file );
  char * template_buffer;
  
  subst_list_update_string( template->arg_list , &template_file);
  if (ext_arg_list != NULL)
    subst_list_update_string( ext_arg_list , &template_file);
  
  template_buffer = util_fread_alloc_file_content( template_file , &buffer_size );
  free( template_file );
  
  return template_buffer;
}



/**
   This function allocates a template object based on the source file
   'template_file'. If @internalize_template is true the template
   content will be read and internalized at boot time, otherwise that
   is deferred to template instantiation time (in which case the
   template file can change dynamically).
*/


template_type * template_alloc( const char * template_file , bool internalize_template , subst_list_type * parent_subst) {
  template_type * template = util_malloc( sizeof * template , __func__);
  UTIL_TYPE_ID_INIT(template , TEMPLATE_TYPE_ID);
  template->arg_list        = subst_list_alloc( parent_subst );
  template->template_buffer = NULL;

  template->template_file        = util_alloc_string_copy( template_file );
  template->internalize_template = internalize_template;
  
  if (template->internalize_template)
    template->template_buffer = template_load( template , NULL );
  
  return template;
}



void template_free( template_type * template ) {
  subst_list_free( template->arg_list );
  util_safe_free( template->template_file );
  util_safe_free( template->template_buffer );
  free( template );
}



/**
   This function will create the file @__target_file based on the
   template instance. Before the target file is written all the
   internal substitutions and then subsequently the subsititutions in
   @arg_list will be performed. The input @arg_list can be NULL - in
   which case this is more like copy operation.

   Observe that:
   
    1. Substitutions will be performed on @__target_file

    2. @__target_file can contain path components.

    3. If internalize_template == false subsititions will be performed
       on the filename of the file with template content.

*/
   


void template_instansiate( const template_type * template , const char * __target_file , const subst_list_type * arg_list ) {
  char * target_file = util_alloc_string_copy( __target_file );

  /* Finding the name of the target file. */
  subst_list_update_string( template->arg_list , &target_file);
  if (arg_list != NULL) subst_list_update_string( arg_list           , &target_file );

  {
    char * buffer;
    /* Loading the template - possibly expanding keys in the filename */
    if (template->internalize_template)
      buffer = util_alloc_string_copy( template->template_buffer);
    else
      buffer = template_load( template , arg_list );
    
    /* Substitutions on the content. */
    subst_list_update_string( template->arg_list , &buffer );
    if (arg_list != NULL) subst_list_update_string( arg_list           , &buffer );

    /* Write the content out. */
    {
      FILE * stream = util_mkdir_fopen( target_file , "w");
      fprintf(stream , "%s" , buffer);
      fclose( stream );
    }
    free( buffer );
  }
}


/**
   Add an internal key_value pair. This substitution will be performed
   before the internal substitutions.
*/
void template_add_arg( template_type * template , const char * key , const char * value ) {
  subst_list_insert_copy( template->arg_list , key , value , NULL /* No doc_string */);
}
