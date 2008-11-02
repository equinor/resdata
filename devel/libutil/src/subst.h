#ifndef __SUBST_H__
#define __SUBST_H__

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct subst_list_struct subst_list_type;

subst_list_type    * subst_list_alloc();
void                 subst_list_free(subst_list_type *);
void                 subst_list_insert_copy(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_ref(subst_list_type *  , const char * , const char * );
void                 subst_list_insert_owned_ref(subst_list_type *  , const char * , const char * );


#ifdef __cplusplus 
}
#endif
#endif



