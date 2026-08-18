#ifndef ERT_TYPE_VECTOR_FUNCTIONS_H
#define ERT_TYPE_VECTOR_FUNCTIONS_H

#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>

#ifdef __cplusplus
extern "C" {
#endif

int_vector_type *bool_vector_alloc_active_list(const bool_vector_type *mask);
bool_vector_type *int_vector_alloc_mask(const int_vector_type *active_list);

#ifdef __cplusplus
}
#endif
#endif
