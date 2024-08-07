#ifndef ERT_HASH_NODE_H
#define ERT_HASH_NODE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include <ert/util/node_data.hpp>

typedef struct hash_node_struct hash_node_type;
typedef uint32_t(hashf_type)(const char *key, size_t len);
typedef enum {
    hash_ref_data,
    hash_int_data,
    hash_double_data,
    hash_string_data
} hash_data_type;

bool hash_node_key_eq(const hash_node_type *, uint32_t, const char *);
hash_node_type *hash_node_get_next(const hash_node_type *);
void hash_node_set_next(hash_node_type *, const hash_node_type *);
hash_node_type *hash_node_alloc_new(const char *, node_data_type *,
                                    hashf_type *, uint32_t);
uint32_t hash_node_get_table_index(const hash_node_type *);
const char *hash_node_get_key(const hash_node_type *);
node_data_type *hash_node_get_data(const hash_node_type *);
void hash_node_free(hash_node_type *);
uint32_t hash_node_set_table_index(hash_node_type *, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
