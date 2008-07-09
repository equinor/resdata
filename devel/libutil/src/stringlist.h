#ifndef __STRINGLIST_H__
#define __STRINGLIST_H__


typedef struct stringlist_struct stringlist_type;


stringlist_type * stringlist_alloc_new();
void              stringlist_free(stringlist_type *);
void              stringlist_append_copy(stringlist_type * , const char *);
void              stringlist_append_ref(stringlist_type * , const char *);
void              stringlist_append_owned_ref(stringlist_type * , const char *);


#endif
