
#include <ert/util/int_vector.hpp>
#include <ert/util/bool_vector.hpp>

#include <ert/util/type_vector_functions.hpp>

int_vector_type *bool_vector_alloc_active_list(const bool_vector_type *mask) {
    int_vector_type *active_list = int_vector_alloc(0, 0);
    int i;

    for (i = 0; i < bool_vector_size(mask); i++)
        if (bool_vector_iget(mask, i))
            int_vector_append(active_list, i);

    return active_list;
}

bool_vector_type *int_vector_alloc_mask(const int_vector_type *active_list) {
    bool_vector_type *mask = bool_vector_alloc(0, false);
    int i;
    for (i = 0; i < int_vector_size(active_list); i++)
        bool_vector_iset(mask, int_vector_iget(active_list, i), true);

    return mask;
}
