#ifndef ERT_ECL_SUM_VECTOR_H
#define ERT_ECL_SUM_VECTOR_H

#include <ert/util/type_macros.hpp>

#include <ert/ecl/ecl_sum.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ecl_sum_vector_struct ecl_sum_vector_type;

void ecl_sum_vector_free(ecl_sum_vector_type *keylist);
ecl_sum_vector_type *ecl_sum_vector_alloc(const ecl_sum_type *ecl_sum,
                                          bool add_keywords);

bool ecl_sum_vector_add_key(ecl_sum_vector_type *keylist, const char *key);
void ecl_sum_vector_add_keys(ecl_sum_vector_type *keylist, const char *pattern);

const char *ecl_sum_vector_iget_key(const ecl_sum_vector_type *ecl_sum_vector,
                                    int index);
bool ecl_sum_vector_iget_is_rate(const ecl_sum_vector_type *ecl_sum_vector,
                                 int index);
int ecl_sum_vector_iget_param_index(const ecl_sum_vector_type *ecl_sum_vector,
                                    int index);
int ecl_sum_vector_get_size(const ecl_sum_vector_type *ecl_sum_vector);
bool ecl_sum_vector_iget_valid(const ecl_sum_vector_type *ecl_sum_vector,
                               int index);

ecl_sum_vector_type *
ecl_sum_vector_alloc_layout_copy(const ecl_sum_vector_type *src_vector,
                                 const ecl_sum_type *ecl_sum);

UTIL_IS_INSTANCE_HEADER(ecl_sum_vector);

#ifdef __cplusplus
}
#endif
#endif
