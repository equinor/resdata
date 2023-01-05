#ifndef ERT_TYPE_VECTOR_FUNCTIONS_H
#define ERT_TYPE_VECTOR_FUNCTIONS_H

#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>
#include <ert/util/double_vector.hpp>

#ifdef __cplusplus
extern "C" {
#endif

int_vector_type *bool_vector_alloc_active_list(const bool_vector_type *mask);
bool_vector_type *int_vector_alloc_mask(const int_vector_type *active_list);
int_vector_type *
bool_vector_alloc_active_index_list(const bool_vector_type *mask,
                                    int default_value);
bool double_vector_approx_equal(const double_vector_type *v1,
                                const double_vector_type *v12, double epsilon);

#ifdef __cplusplus
}
#endif
#endif
