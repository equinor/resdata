#ifndef __STRINGLIST_H__
#define __STRINGLIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <int_vector.h>
#include <subst.h>
#include <buffer.h>

typedef struct stringlist_struct stringlist_type;


stringlist_type * stringlist_alloc_new();
void              stringlist_free__(void * );
void              stringlist_free(stringlist_type *);
void              stringlist_clear(stringlist_type * );

void              stringlist_append_copy(stringlist_type * , const char *);
void              stringlist_append_ref(stringlist_type * , const char *);
void              stringlist_append_owned_ref(stringlist_type * , const char *);

const      char * stringlist_safe_iget( const stringlist_type * stringlist , int index);
bool              stringlist_iequal( const stringlist_type * stringlist , int index, const char * s );
const      char * stringlist_iget(const stringlist_type * , int);
char            * stringlist_iget_copy(const stringlist_type * stringlist , int );
char            * stringlist_alloc_joined_string(const stringlist_type *  , const char * );
char            * stringlist_alloc_joined_segment_string( const stringlist_type * s , int start_index , int end_index , const char * sep );

void 		  stringlist_iset_copy(stringlist_type *, int index , const char *);
void 		  stringlist_iset_ref(stringlist_type *, int index , const char *);
void 		  stringlist_iset_owned_ref(stringlist_type *, int index , const char *);
void              stringlist_idel(stringlist_type * stringlist , int index);

stringlist_type * stringlist_alloc_argv_copy(const char **      , int );
stringlist_type * stringlist_alloc_argv_ref (const char **      , int );
stringlist_type * stringlist_alloc_argv_owned_ref(const char ** argv , int argc);
int               stringlist_get_size(const stringlist_type * );
void              stringlist_fprintf(const stringlist_type * , const char * , FILE *);
stringlist_type * stringlist_alloc_shallow_copy(const stringlist_type *);
stringlist_type * stringlist_alloc_deep_copy(const stringlist_type *);
stringlist_type * stringlist_alloc_shallow_copy_with_offset(const stringlist_type * stringlist, int offset);
stringlist_type * stringlist_alloc_shallow_copy_with_limits(const stringlist_type * stringlist, int start, int num_strings);
 

void              stringlist_append_stringlist_copy(stringlist_type *  , const stringlist_type * );
void              stringlist_append_stringlist_ref(stringlist_type *   , const stringlist_type * );
void              stringlist_insert_stringlist_copy(stringlist_type *  , const stringlist_type *, int);

bool              stringlist_equal(const stringlist_type *  , const stringlist_type *);
bool              stringlist_contains(const stringlist_type *  , const char * );
int_vector_type * stringlist_find(const stringlist_type *, const char *);
int               stringlist_find_first(const stringlist_type * , const char * );
int   	          stringlist_get_argc(const stringlist_type * );
char           ** stringlist_alloc_char_copy(const stringlist_type * );
void              stringlist_fread(stringlist_type * , FILE * );
void              stringlist_fwrite(const stringlist_type * , FILE * );
void              stringlist_buffer_fread( stringlist_type * s , buffer_type * buffer );
void              stringlist_buffer_fwrite( const stringlist_type * s , buffer_type * buffer );
stringlist_type * stringlist_fread_alloc(FILE * );
void              stringlist_sort(stringlist_type *);
void              stringlist_apply_subst(stringlist_type * stringlist , const subst_list_type * subst_list); 

#ifdef __cplusplus
}
#endif
#endif 
