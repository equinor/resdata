#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <ert/util/hash.hpp>
#include <ert/util/hash_sll.hpp>
#include <ert/util/hash_node.hpp>
#include <ert/util/node_data.hpp>
#include <ert/util/util.hpp>
#include <ert/util/stringlist.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#define HASH_DEFAULT_SIZE 16
#define HASH_TYPE_ID 771065

/**
   Implements the jenkins one-at-a-time hash function

   see  https://en.wikipedia.org/wiki/Jenkins_hash_function
*/

static uint32_t hash_index(const char *key, size_t len) {
    uint32_t hash = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

struct hash_struct {
    UTIL_TYPE_ID_DECLARATION;
    uint32_t
        size; /* This is the size of the internal table **NOT**NOT** the number of elements in the table. */
    uint32_t elements; /* The number of elements in the hash table. */
    double resize_fill;
    hash_sll_type **table;
    hashf_type *hashf;
};

static void *__hash_get_node(const hash_type *__hash, const char *key,
                             bool abort_on_error) {
    hash_type *hash =
        (hash_type *)__hash; /* The net effect is no change - but .... ?? */
    hash_node_type *node = NULL;
    {
        const uint32_t global_index = hash->hashf(key, strlen(key));
        const uint32_t table_index = (global_index % hash->size);

        node = hash_sll_get(hash->table[table_index], global_index, key);
        if (node == NULL && abort_on_error)
            util_abort("%s: tried to get from key:%s which does not exist - "
                       "aborting \n",
                       __func__, key);
    }
    return node;
}

static node_data_type *hash_get_node_data(const hash_type *hash,
                                          const char *key) {
    hash_node_type *node = (hash_node_type *)__hash_get_node(hash, key, true);
    return hash_node_get_data(node);
}

/**
   This function resizes the hash table when it has become to full.
   The table only grows - this function is called from
   __hash_insert_node().

   If you know in advance (roughly) how large the hash table will be
   it can be advantageous to call hash_resize() manually, to avoid
   repeated internal calls to hash_resize().
*/

void hash_resize(hash_type *hash, int new_size) {
    hash_sll_type **new_table = hash_sll_alloc_table(new_size);
    hash_node_type *node;
    uint32_t i;

    for (i = 0; i < hash->size; i++) {
        node = hash_sll_get_head(hash->table[i]);
        while (node != NULL) {
            uint32_t new_table_index =
                hash_node_set_table_index(node, new_size);
            hash_node_type *next_node = hash_node_get_next(node);
            hash_sll_add_node(new_table[new_table_index], node);
            node = next_node;
        }
    }

    /*
     Only freeing the table structure, *NOT* calling the node_free()
     functions, which happens when hash_sll_free() is called.
  */

    {
        for (i = 0; i < hash->size; i++)
            free(hash->table[i]);
        free(hash->table);
    }

    hash->size = new_size;
    hash->table = new_table;
}

/**
   This is the low-level function for inserting a hash node.
*/

static void __hash_insert_node(hash_type *hash, hash_node_type *node) {
    uint32_t table_index = hash_node_get_table_index(node);
    {
        /*
      If a node with the same key already exists in the table
      it is removed.
    */
        hash_node_type *existing_node = (hash_node_type *)__hash_get_node(
            hash, hash_node_get_key(node), false);
        if (existing_node != NULL) {
            hash_sll_del_node(hash->table[table_index], existing_node);
            hash->elements--;
        }
    }

    hash_sll_add_node(hash->table[table_index], node);
    hash->elements++;
    if ((1.0 * hash->elements / hash->size) > hash->resize_fill)
        hash_resize(hash, hash->size * 2);
}

/**
   This functions takes a hash_node and finds the "next" hash node by
   traversing the internal hash structure. Should NOT be confused with
   the other functions providing iterations to user-space.
*/

static hash_node_type *hash_internal_iter_next(const hash_type *hash,
                                               const hash_node_type *node) {
    hash_node_type *next_node = hash_node_get_next(node);
    if (next_node == NULL) {
        const uint32_t table_index = hash_node_get_table_index(node);
        if (table_index < hash->size) {
            uint32_t i = table_index + 1;
            while (i < hash->size && hash_sll_empty(hash->table[i]))
                i++;

            if (i < hash->size)
                next_node = hash_sll_get_head(hash->table[i]);
        }
    }
    return next_node;
}

/**
   This is the low level function which traverses a hash table and
   allocates a char ** list of keys.

   If the hash table is empty NULL is returned.
*/

static char **hash_alloc_keylist__(const hash_type *hash) {
    char **keylist;
    if (hash->elements > 0) {
        int i = 0;
        hash_node_type *node = NULL;
        keylist = (char **)calloc(hash->elements, sizeof *keylist);
        {
            uint32_t i = 0;
            while (i < hash->size && hash_sll_empty(hash->table[i]))
                i++;

            if (i < hash->size)
                node = hash_sll_get_head(hash->table[i]);
        }

        while (node != NULL) {
            const char *key = hash_node_get_key(node);
            keylist[i] = util_alloc_string_copy(key);
            node = hash_internal_iter_next(hash, node);
            i++;
        }
    } else
        keylist = NULL;
    return keylist;
}

void hash_insert_string(hash_type *hash, const char *key, const char *value) {
    node_data_type *node_data = node_data_alloc_string(value);
    hash_node_type *hash_node =
        hash_node_alloc_new(key, node_data, hash->hashf, hash->size);
    __hash_insert_node(hash, hash_node);
}

char *hash_get_string(const hash_type *hash, const char *key) {
    node_data_type *node_data = hash_get_node_data(hash, key);
    return node_data_get_string(node_data);
}

void hash_insert_int(hash_type *hash, const char *key, int value) {
    node_data_type *node_data = node_data_alloc_int(value);
    hash_node_type *hash_node =
        hash_node_alloc_new(key, node_data, hash->hashf, hash->size);
    __hash_insert_node(hash, hash_node);
}

int hash_get_int(const hash_type *hash, const char *key) {
    node_data_type *node_data = hash_get_node_data(hash, key);
    return node_data_get_int(node_data);
}

void hash_insert_double(hash_type *hash, const char *key, double value) {
    node_data_type *node_data = node_data_alloc_double(value);
    hash_node_type *hash_node =
        hash_node_alloc_new(key, node_data, hash->hashf, hash->size);
    __hash_insert_node(hash, hash_node);
}

double hash_get_double(const hash_type *hash, const char *key) {
    node_data_type *node_data = hash_get_node_data(hash, key);
    return node_data_get_double(node_data);
}

/**
   This function will delete the key if it exists in the hash, but it
   will NOT fail if the key is not already in the hash table.
*/

void *hash_get(const hash_type *hash, const char *key) {
    hash_node_type *hash_node =
        (hash_node_type *)__hash_get_node(hash, key, true);
    node_data_type *data_node = (node_data_type *)hash_node_get_data(hash_node);
    return node_data_get_ptr(data_node);
}

/**
   This function will return NULL if the hash does not
   contain 'key'.
*/
/**
   This function will:

    1. Return an object from the hash table.
    2. Remove it from the table.
*/

static hash_type *__hash_alloc(int size, double resize_fill,
                               hashf_type *hashf) {
    hash_type *hash;
    hash = (hash_type *)util_malloc(sizeof *hash);
    UTIL_TYPE_ID_INIT(hash, HASH_TYPE_ID);
    hash->size = size;
    hash->hashf = hashf;
    hash->table = hash_sll_alloc_table(hash->size);
    hash->elements = 0;
    hash->resize_fill = resize_fill;
    return hash;
}

hash_type *hash_alloc() {
    return __hash_alloc(HASH_DEFAULT_SIZE, 0.50, hash_index);
}

UTIL_SAFE_CAST_FUNCTION(hash, HASH_TYPE_ID)
UTIL_SAFE_CAST_FUNCTION_CONST(hash, HASH_TYPE_ID)
UTIL_IS_INSTANCE_FUNCTION(hash, HASH_TYPE_ID)

void hash_free(hash_type *hash) {
    uint32_t i;
    for (i = 0; i < hash->size; i++)
        hash_sll_free(hash->table[i]);
    free(hash->table);
    free(hash);
}

char **hash_alloc_keylist(const hash_type *hash) {
    return hash_alloc_keylist__(hash);
}

stringlist_type *hash_alloc_stringlist(const hash_type *hash) {
    stringlist_type *stringlist = stringlist_alloc_new();
    char **keylist = hash_alloc_keylist__(hash);
    int i;
    for (i = 0; i < hash_get_size(hash); i++) {
        stringlist_append_copy(stringlist, keylist[i]);
        free(keylist[i]);
    }

    free(keylist);
    return stringlist;
}

/** Inserts an entry in the hash table so that the hash table takes ownership
  of 'value', in the sense that the hash table will call the
  'destructor' del() on value when the node is deleted.

  This function will insert a reference "value" with key "key"; when
  the key is deleted -
  when the complete hash table is free'd with hash_free(), the
  destructur 'del' is called with 'value' as argument.

  It is importand to realize that when elements are inserted into a
  hash table with this function the calling scope gives up
  responsibility of freeing the memory pointed to by value.
*/

void hash_insert_hash_owned_ref(hash_type *hash, const char *key,
                                const void *value, free_ftype *del) {
    hash_node_type *hash_node;
    if (del == NULL)
        util_abort("%s: must provide delete operator for insert hash_owned_ref "
                   "- aborting \n",
                   __func__);
    {
        node_data_type *data_node = node_data_alloc_ptr(value, NULL, del);
        hash_node =
            hash_node_alloc_new(key, data_node, hash->hashf, hash->size);
        __hash_insert_node(hash, hash_node);
    }
}

/** Inserts an entry in the hash table so that the hash table ONLY contains a
    pointer to value; the calling scope retains full ownership to
    value. When the hash node is deleted, the hash implementation
    will just drop the reference on the floor.
 */
void hash_insert_ref(hash_type *hash, const char *key, const void *value) {
    hash_node_type *hash_node;
    {
        node_data_type *data_node = node_data_alloc_ptr(value, NULL, NULL);
        hash_node =
            hash_node_alloc_new(key, data_node, hash->hashf, hash->size);
        __hash_insert_node(hash, hash_node);
    }
}

bool hash_has_key(const hash_type *hash, const char *key) {
    if (__hash_get_node(hash, key, false) == NULL)
        return false;
    else
        return true;
}

int hash_get_size(const hash_type *hash) { return hash->elements; }

#undef HASH_GET_SCALAR
#undef HASH_INSERT_SCALAR
#undef HASH_INSERT_ARRAY
#undef HASH_GET_ARRAY_PTR
#undef HASH_NODE_AS
#undef HASH_DEFAULT_SIZE

#ifdef __cplusplus
}
#endif
