#ifndef __STRINGLIST_H__
#define __STRINGLIST_H__


typedef struct stringlist_struct stringlist_type;


stringlist_type * stringlist_alloc_new();
void              stringlist_free(stringlist_type *);
void              stringlist_append_copy(stringlist_type * , const char *);
void              stringlist_append_ref(stringlist_type * , const char *);
void              stringlist_append_owned_ref(stringlist_type * , const char *);
const      char * stringlist_iget(const stringlist_type * , int);

void stringlist_iset_copy(stringlist_type *, int index , const char *);
void stringlist_iset_ref(stringlist_type *, int index , const char *);
void stringlist_iset_owned_ref(stringlist_type *, int index , const char *);

stringlist_type * stringlist_alloc_argv_copy(const char **      , int );
stringlist_type * stringlist_alloc_argv_ref (const char **      , int );
stringlist_type * stringlist_alloc_argv_owned_ref(const char ** , int );
int               stringlist_get_size(const stringlist_type * );


#endif
