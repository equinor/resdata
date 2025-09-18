#ifndef ERT_HASH_H
#define ERT_HASH_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include <ert/util/stringlist.hpp>
#include <ert/util/type_macros.hpp>
#include <ert/util/node_data.hpp>

typedef struct hash_struct hash_type;
typedef struct hash_iter_struct hash_iter_type;

UTIL_SAFE_CAST_HEADER(hash);
UTIL_SAFE_CAST_HEADER_CONST(hash);

hash_type *hash_alloc(void);
void hash_free(hash_type *);
void hash_insert_ref(hash_type *, const char *, const void *);
void hash_insert_string(hash_type *, const char *, const char *);
bool hash_has_key(const hash_type *, const char *);
void *hash_get(const hash_type *, const char *);
char *hash_get_string(const hash_type *, const char *);
int hash_get_size(const hash_type *);
stringlist_type *hash_alloc_stringlist(const hash_type *);
void hash_insert_hash_owned_ref(hash_type *, const char *, const void *,
                                free_ftype *);
void hash_resize(hash_type *hash, int new_size);

void hash_insert_int(hash_type *, const char *, int);
int hash_get_int(const hash_type *, const char *);
void hash_insert_double(hash_type *, const char *, double);
double hash_get_double(const hash_type *, const char *);

UTIL_IS_INSTANCE_HEADER(hash);

#ifdef __cplusplus
}
#endif
#endif
