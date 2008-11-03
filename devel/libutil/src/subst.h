#ifndef __SUBST_H__
#define __SUBST_H__

#ifdef __cplusplus 
extern "C" {
#endif
#include <stdio.h>

typedef struct subst_list_struct subst_list_type;

subst_list_type    * subst_list_alloc();
void                 subst_list_free(subst_list_type *);
void                 subst_list_insert_copy(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_ref(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_owned_ref(subst_list_type *  , const char * , const char * );

void 	subst_list_filter_file(const subst_list_type * , const char * , const  char * );
void 	subst_list_update_file(const subst_list_type * , const char * );
void    subst_list_update_string(const subst_list_type * , char ** );
char  * subst_list_alloc_filtered_string(const subst_list_type * , const char * );
void    subst_list_filtered_fprintf(const subst_list_type * , const char *  , FILE * );



#ifdef __cplusplus 
}
#endif
#endif



