#pragma once

#include <cstddef>
#include <vector>
#include <memory>

typedef struct nnc_vector_struct nnc_vector_type;

const std::vector<int> &
nnc_vector_get_grid_index_list(const nnc_vector_type *nnc_vector);
const std::vector<int> &
nnc_vector_get_nnc_index_list(const nnc_vector_type *nnc_vector);

int nnc_vector_iget_nnc_index(const nnc_vector_type *nnc_vector, size_t index);
int nnc_vector_iget_grid_index(const nnc_vector_type *nnc_vector, size_t index);
nnc_vector_type *nnc_vector_alloc(int lgr_nr);
nnc_vector_type *nnc_vector_alloc_copy(const nnc_vector_type *src_vector);
void nnc_vector_free(nnc_vector_type *nnc_vector);
void nnc_vector_add_nnc(nnc_vector_type *nnc_vector, int global_cell_number,
                        int nnc_index);
int nnc_vector_get_lgr_nr(const nnc_vector_type *nnc_vector);
size_t nnc_vector_get_size(const nnc_vector_type *nnc_vector);
bool nnc_vector_equal(const nnc_vector_type *nnc_vector1,
                      const nnc_vector_type *nnc_vector2);

using nnc_vector_ptr =
    std::unique_ptr<nnc_vector_type, decltype(&nnc_vector_free)>;

inline nnc_vector_ptr make_nnc_vector(int lgr_nr) {
    return {nnc_vector_alloc(lgr_nr), &nnc_vector_free};
}
