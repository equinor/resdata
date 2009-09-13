#include <stdlib.h>
#include <stdbool.h>
#include <subst.h>
#include <template.h>

#define TEMPLATE_TYPE_ID 7781045

struct template_struct {
  UTIL_TYPE_ID_DECLARATION;
  char            * template_file;
  char            * template_buffer;
  bool              internalize_template;
  subst_list_type * arg_list; 
};


static void template_load( template_type * template ) {
  int buffer_size;
  template->template_buffer = util_fread_alloc_file_content( template->template_file , &buffer_size );
}



template_type * template_alloc( const char * template_file , bool internalize_template) {
  template_type * template = util_malloc( sizeof * template , __func__);
  UTIL_TYPE_ID_INIT(template , TEMPLATE_TYPE_ID);
  template->arg_list        = subst_list_alloc();
  template->template_buffer = NULL;

  template->template_file        = util_alloc_string_copy( template_file );
  template->internalize_template = internalize_template;
  
  if (template->internalize_template)
    template_load( template );
  return template;
}



void template_free( template_type * template ) {
  subst_list_free( template->arg_list );
  util_safe_free( template->template_file );
  util_safe_free( template->template_buffer );
  free( template );
}



void template_instansiate( template_type * template , const char * __target_file , const subst_list_type * arg_list ) {
  char * target_file = util_alloc_string_copy( __target_file );
  if (!template->internalize_template)
    template_load( template );

  subst_list_update_string( template->arg_list , &target_file);
  subst_list_update_string( arg_list , &target_file );
  {
    char * buffer = util_alloc_string_copy( template->template_buffer );
    subst_list_update_string( template->arg_list , &buffer );
    subst_list_update_string( arg_list , &buffer );
    {
      FILE * stream = util_mkdir_fopen( target_file , "w");
      fprintf(stream , "%s" , buffer);
      fclose( stream );
    }
    free( buffer );
  }
  if (!template->internalize_template) {
    free(template->template_buffer);
    template->template_buffer = NULL;
  }
}



void template_add_arg( template_type * template , const char * key , const char * value ) {
  subst_list_insert_copy( template->arg_list , key , value );
}
