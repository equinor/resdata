#ifndef __HASH_H__
#define __HASH_H__
#ifdef __cplusplus
extern"C" {
#endif

#include <stdlib.h>
#include <stringlist.h>
typedef struct hash_struct      hash_type;
typedef struct hash_iter_struct hash_iter_type;

#include <hash_node.h>

void              hash_lock  (hash_type * );
void              hash_unlock(hash_type * );
hash_type       * hash_alloc();
void              hash_iter_complete(hash_type * );
void              hash_free(hash_type *);
void              hash_free__(void *);
void              hash_insert_ref(hash_type * , const char * , const void *);
void              hash_insert_copy(hash_type *, const char * , const void *, copyc_type *, del_type *);
void              hash_insert_string(hash_type *, const char *, const char *);
bool              hash_has_key(const hash_type *, const char *);
void            * hash_get(const hash_type *, const char *);
char            * hash_get_string(hash_type * , const char *);
void              hash_del(hash_type *, const char *);
void              hash_safe_del(hash_type * , const char * );
void              hash_clear(hash_type *);
int               hash_get_size(const hash_type *);
void              hash_set_keylist(const hash_type * , char **);
char           ** hash_alloc_keylist(hash_type *);

char           ** hash_alloc_sorted_keylist (hash_type *hash , int ( hash_get_cmp_value ) (const void *));
char           ** hash_alloc_key_sorted_list(hash_type *hash, int (*cmp)(const void *, const void *));
bool              hash_key_list_compare( hash_type * hash1, hash_type * hash2);
void              hash_insert_hash_owned_ref(hash_type *, const char * , const void *, del_type *);


hash_iter_type  * hash_iter_alloc(const hash_type *);
void              hash_iter_free(hash_iter_type *);
bool              hash_iter_is_complete(hash_iter_type *);
const      char * hash_iter_get_next_key(hash_iter_type *);
void            * hash_iter_get_next_value(hash_iter_type *);


hash_type       * hash_alloc_from_options(const stringlist_type *);


void              hash_insert_int(hash_type * , const char * , int);
int               hash_get_int(hash_type * , const char *);
void              hash_insert_double(hash_type * , const char * , double);
double               hash_get_double(hash_type * , const char *);

#ifdef __cplusplus
}
#endif
#endif
