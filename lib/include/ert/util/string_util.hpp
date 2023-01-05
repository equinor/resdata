#ifndef ERT_STRING_UTIL_H
#define ERT_STRING_UTIL_H

#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>

#ifdef __cplusplus
extern "C" {
#endif

bool string_util_init_active_list(const char *range_string,
                                  int_vector_type *active_list);
bool string_util_update_active_list(const char *range_string,
                                    int_vector_type *active_list);
int_vector_type *string_util_alloc_active_list(const char *range_string);

bool string_util_init_active_mask(const char *range_string,
                                  bool_vector_type *active_mask);
bool string_util_update_active_mask(const char *range_string,
                                    bool_vector_type *active_mask);
bool_vector_type *string_util_alloc_active_mask(const char *range_string);

bool string_util_update_value_list(const char *range_string,
                                   int_vector_type *value_list);
bool string_util_init_value_list(const char *range_string,
                                 int_vector_type *value_list);
int_vector_type *string_util_alloc_value_list(const char *range_string);

#ifdef __cplusplus
}
#endif
#endif
