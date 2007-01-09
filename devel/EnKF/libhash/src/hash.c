#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <hash.h>
#include <hash_sll.h>
#include <hash_node.h>

struct hash_struct {
  int             size;
  int             elements;
  double          resize_fill;
  hash_sll_type **table;
  hashf_type     *hashf;
};

typedef struct hash_data_struct {
  int    byte_size;
  void  *data;
} hash_data_type;

/*****************************************************************/

#define HASH_GET_SCALAR(FUNC,TYPE) \
TYPE FUNC (const hash_type *hash,  const char *key) { \
   hash_data_type *data = hash_get(hash , key);       \
   return *((TYPE *) data->data);                     \
}


#define HASH_INSERT_SCALAR(FUNC,TYPE) \
void FUNC(hash_type *hash , const char *key , TYPE value) {     \
  hash_insert_managed_copy(hash , key , &value , sizeof value); \
}


#define HASH_INSERT_ARRAY(FUNC,TYPE)                                   \
void FUNC(hash_type *hash, const char *key , TYPE *value, int SIZE) {  \
  hash_insert_managed_copy(hash , key , value , SIZE * sizeof *value); \
}

#define HASH_GET_ARRAY_PTR(FUNC,TYPE)\
TYPE * FUNC(const hash_type * hash, const char *key) { \
   hash_data_type *data = hash_get(hash , key);       \
   return ((TYPE *) data->data);                      \
}

/*****************************************************************/

static const void * hash_data_copyc(const void *_src) {
  const hash_data_type *src = (const hash_data_type *) _src;
  hash_data_type *new;
  new = malloc(sizeof *new);
  new->byte_size = src->byte_size;
  new->data = malloc(new->byte_size);
  memcpy(new->data , src->data , new->byte_size);
  return new;
}


static void hash_data_free(void *_hash_data) {
  hash_data_type *hash_data = (hash_data_type *) _hash_data;
  free(hash_data->data);
  free(hash_data);
}


static void * __hash_get_node(const hash_type *hash , const char *key, bool abort_on_error) {
  const uint32_t global_index = hash->hashf(key , strlen(key));
  const uint32_t table_index  = (global_index % hash->size);
  hash_node_type *node        = hash_sll_get(hash->table[table_index] , global_index , key);
  if (node != NULL)
    return node;
  else {
    if (abort_on_error) {
      fprintf(stderr,"%s: tried to get from key:%s which does not exist - aborting \n",__func__ , key);
      abort();
    } else return NULL;
  }
}


void hash_del(hash_type *hash , const char *key) {
  const uint32_t global_index = hash->hashf(key , strlen(key));
  const uint32_t table_index  = (global_index % hash->size);
  hash_node_type *node        = hash_sll_get(hash->table[table_index] , global_index , key);
  if (node == NULL) {
    fprintf(stderr,"%s: hash does not contain key:%s - aborting \n",__func__ , key);
    abort();
  } else
  hash_sll_del_node(hash->table[table_index] , node);
  hash->elements--;
}



void * hash_get(const hash_type *hash , const char *key) {
  hash_node_type * node = __hash_get_node(hash , key , true);
  return hash_node_value_ptr(node);
}


HASH_GET_SCALAR(hash_get_int    , int)
HASH_GET_SCALAR(hash_get_double , double)
HASH_GET_ARRAY_PTR(hash_get_double_ptr , double)
HASH_GET_ARRAY_PTR(hash_get_int_ptr    , int)


static uint32_t hash_index(const uint8_t *key, size_t len) {
  uint32_t hash = 0;
  size_t i;

  for (i=0; i < len; i++) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >>  6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}



static hash_type * __hash_alloc(int size, double resize_fill , hashf_type *hashf) {
  hash_type* hash;
  hash = malloc(sizeof *hash);
  hash->size  	 = size;
  hash->hashf 	 = hashf;
  hash->table 	 = hash_sll_alloc_table(hash->size);
  hash->elements = 0;
  hash->resize_fill = resize_fill;
  return hash;
}

static void hash_resize(hash_type *hash, int new_size) {
  hash_sll_type **new_table = hash_sll_alloc_table(new_size);
  hash_node_type *node;
  int i;
  
  for (i=0; i < hash->size; i++) {
    node = hash_sll_get_head(hash->table[i]);
    while (node != NULL) {
      uint32_t new_table_index  = hash_node_set_table_index(node , new_size);
      hash_node_type *next_node = hash_node_get_next(node);
      hash_sll_add_node(new_table[new_table_index] , node);
      node = next_node;
    }
  }
  
  free(hash->table);
  hash->size     = new_size;
  hash->table    = new_table;
}



hash_type * hash_alloc(int size) {
  return __hash_alloc(size , 0.50 , hash_index);
}


void hash_free(hash_type *hash) {
  int i;
  for (i=0; i < hash->size; i++) 
    hash_sll_free(hash->table[i]);
  free(hash->table);
  free(hash);
}


static void hash_insert_node(hash_type *hash , hash_node_type *node) {
  uint32_t table_index = hash_node_get_table_index(node);
  {
    /*
      If a node with the same key already exists in the table
      it is removed.
    */
    hash_node_type *existing_node = __hash_get_node(hash , hash_node_get_keyref(node) , false);
    if (existing_node != NULL) {
      hash_sll_del_node(hash->table[table_index] , existing_node);
      hash->elements--;
    }
  }

  hash_sll_add_node(hash->table[table_index] , node);  
  hash->elements++;
  if ((1.0 * hash->elements / hash->size) > hash->resize_fill) 
    hash_resize(hash , hash->size * 2);
}


static void hash_insert_managed_copy(hash_type *hash, const char *key, const void *value_ptr , int value_size) {
  hash_node_type *node;
  hash_data_type hash_data;
  hash_data.data      = (void *) value_ptr;
  hash_data.byte_size = value_size;
  node = hash_node_alloc_new(key , &hash_data , hash_data_copyc , hash_data_free , hash->hashf , hash->size);
  hash_insert_node(hash , node);
}




/*****************************************************************/

void hash_insert_copy(hash_type *hash , const char *key , const void *value , copyc_type *copyc , del_type *del) {
  hash_node_type *node;
  if (copyc == NULL || del == NULL) {
    fprintf(stderr,"%s: must provide copy constructer and delete operator for insert copy - aborting \n",__func__);
    abort();
  }
  node = hash_node_alloc_new(key , value , copyc , del , hash->hashf , hash->size);
  hash_insert_node(hash , node);
}

void hash_insert_ref(hash_type *hash , const char *key , const void *value) {
  hash_node_type *node;
  node = hash_node_alloc_new(key , value , NULL , NULL , hash->hashf , hash->size);
  hash_insert_node(hash , node);
}

void hash_insert_string_copy(hash_type *hash, const char *key , const char *value) {
  hash_insert_managed_copy(hash , key , value , strlen(value) + 1);
}



HASH_INSERT_SCALAR(hash_insert_int          , int)
HASH_INSERT_SCALAR(hash_insert_double       , double)
HASH_INSERT_ARRAY (hash_insert_int_array    , int)
HASH_INSERT_ARRAY (hash_insert_double_array , double)


char * hash_get_string(const hash_type *hash , const char *key) {
  hash_data_type *data = hash_get(hash , key);
  if (data != NULL)
    return data->data;
  else
    return NULL;
}


bool hash_has_key(hash_type *hash , const char *key) {
  if (__hash_get_node(hash , key,false) == NULL)
    return false;
  else
    return true;
}


static hash_node_type * hash_iter_init(const hash_type *hash) {
  uint32_t i = 0;
  while (i < hash->size && hash_sll_empty(hash->table[i]))
    i++;

  if (i < hash->size) 
    return hash_sll_get_head(hash->table[i]);
  else
    return NULL;
}


static hash_node_type * hash_iter_next(const hash_type *hash , const hash_node_type * node) {
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


void hash_printf_keys(const hash_type *hash) {
  hash_node_type *node = hash_iter_init(hash);
  while (node != NULL) {
    hash_node_printf_key(node);
    node = hash_iter_next(hash , node);
  }
}


char ** hash_alloc_keylist(const hash_type *hash) {
  char **keylist;
  int i = 0;
  hash_node_type *node;
  keylist = calloc(hash->elements , sizeof *keylist);
  node = hash_iter_init(hash);
  while (node != NULL) {
    const char *key = hash_node_get_keyref(node); 
    int len = strlen(key);
    keylist[i] = malloc(len + 1);
    strcpy(keylist[i] , key);
    keylist[i][len] = '\0';
    node = hash_iter_next(hash , node);
    i++;
  }
  return keylist;
}


void hash_set_keylist(const hash_type *hash , char **keylist) {
  int i = 0;
  hash_node_type *node;
  node = hash_iter_init(hash);
  while (node != NULL) {
    const char *key = hash_node_get_keyref(node); 
    int len = strlen(key);
    strcpy(keylist[i] , key);
    keylist[i][len] = '\0';
    node = hash_iter_next(hash , node);
    i++;
  }
}
  
int hash_get_size(const hash_type *hash) {
  return hash->elements;
}


#undef HASH_GET_SCALAR
#undef HASH_INSERT_SCALAR
#undef HASH_INSERT_ARRAY
#undef HASH_GET_ARRAY_PTR
