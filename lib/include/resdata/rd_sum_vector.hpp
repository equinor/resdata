#ifndef ERT_RD_SUM_VECTOR_H
#define ERT_RD_SUM_VECTOR_H

#include <ert/util/type_macros.hpp>

#include <resdata/rd_sum.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_sum_vector_struct rd_sum_vector_type;

void rd_sum_vector_free(rd_sum_vector_type *keylist);
rd_sum_vector_type *rd_sum_vector_alloc(const rd_sum_type *rd_sum,
                                        bool add_keywords);

bool rd_sum_vector_add_key(rd_sum_vector_type *keylist, const char *key);
void rd_sum_vector_add_keys(rd_sum_vector_type *keylist, const char *pattern);

const char *rd_sum_vector_iget_key(const rd_sum_vector_type *rd_sum_vector,
                                   int index);
bool rd_sum_vector_iget_is_rate(const rd_sum_vector_type *rd_sum_vector,
                                int index);
int rd_sum_vector_iget_param_index(const rd_sum_vector_type *rd_sum_vector,
                                   int index);
int rd_sum_vector_get_size(const rd_sum_vector_type *rd_sum_vector);
bool rd_sum_vector_iget_valid(const rd_sum_vector_type *rd_sum_vector,
                              int index);

rd_sum_vector_type *
rd_sum_vector_alloc_layout_copy(const rd_sum_vector_type *src_vector,
                                const rd_sum_type *rd_sum);

UTIL_IS_INSTANCE_HEADER(rd_sum_vector);

#ifdef __cplusplus
}
#endif
#endif
