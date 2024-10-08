#ifndef ERT_VECTOR_H
#define ERT_VECTOR_H

#include <ert/util/node_data.hpp>
#include <ert/util/type_macros.hpp>
#include <ert/util/int_vector.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(vector_func_type)(void *, void *);
typedef int(vector_cmp_ftype)(const void *, const void *);

typedef struct vector_struct vector_type;

vector_type *vector_alloc_new(void);

int vector_append_ref(vector_type *, const void *);
int vector_append_owned_ref(vector_type *, const void *, free_ftype *del);
void vector_iset_ref(vector_type *, int, const void *);
void vector_iset_owned_ref(vector_type *, int, const void *, free_ftype *del);
void vector_clear(vector_type *vector);
void vector_free(vector_type *);
void vector_free__(void *arg);
void vector_append_buffer(vector_type *, const void *, int);
void *vector_safe_iget(const vector_type *vector, int index);
const void *vector_iget_const(const vector_type *, int);
void *vector_iget(const vector_type *, int);
void vector_idel(vector_type *vector, int index);
void vector_shrink(vector_type *vector, int new_size);
void *vector_get_last(const vector_type *);
int vector_get_size(const vector_type *);
void *vector_pop_back(vector_type *);
void vector_sort(vector_type *vector, vector_cmp_ftype *cmp);
int_vector_type *vector_alloc_sort_perm(const vector_type *vector,
                                        vector_cmp_ftype *cmp);
void vector_permute(vector_type *vector, const int_vector_type *perm_vector);
void vector_inplace_reverse(vector_type *vector);
vector_type *vector_alloc_copy(const vector_type *src, bool deep_copy);

void vector_iset_buffer(vector_type *vector, int index, const void *buffer,
                        int buffer_size);
UTIL_IS_INSTANCE_HEADER(vector);
UTIL_SAFE_CAST_HEADER(vector);

#ifdef __cplusplus
}
#endif
#endif
