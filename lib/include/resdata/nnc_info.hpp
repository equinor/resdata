#pragma once
#include <cstddef>
#include <vector>
#include <memory>

#include <resdata/nnc_vector.hpp>

typedef struct nnc_info_struct nnc_info_type;

const std::vector<size_t> &
nnc_info_get_grid_index_list(const nnc_info_type *nnc_info, size_t lgr_nr);
const std::vector<size_t> &
nnc_info_get_self_grid_index_list(const nnc_info_type *nnc_info);

nnc_info_type *nnc_info_alloc(size_t lgr_nr);
void nnc_info_free(nnc_info_type *nnc_info);
void nnc_info_add_nnc(nnc_info_type *nnc_info, size_t lgr_nr,
                      size_t global_cell_number, size_t nnc_index);

nnc_vector_type *nnc_info_iget_vector(const nnc_info_type *nnc_info,
                                      size_t lgr_index);

nnc_vector_type *nnc_info_get_vector(const nnc_info_type *nnc_info,
                                     size_t lgr_nr);

nnc_vector_type *nnc_info_get_self_vector(const nnc_info_type *nnc_info);

size_t nnc_info_get_lgr_nr(const nnc_info_type *nnc_info);
size_t nnc_info_get_size(const nnc_info_type *nnc_info);
size_t nnc_info_get_total_size(const nnc_info_type *nnc_info);
bool nnc_info_equal(const nnc_info_type *nnc_info1,
                    const nnc_info_type *nnc_info2);
nnc_info_type *nnc_info_alloc_copy(const nnc_info_type *src_info);
bool nnc_info_has_grid_index_list(const nnc_info_type *nnc_info, size_t lgr_nr);

using nnc_info_ptr = std::unique_ptr<nnc_info_type, decltype(&nnc_info_free)>;
