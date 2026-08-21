#include <cstdlib>
#include <cstdio>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include <resdata/nnc_info.hpp>
#include <resdata/nnc_vector.hpp>

struct nnc_info_struct {
    std::vector<nnc_vector_ptr> lgr_list; /*List of nnc_vectors for LGRs*/
    std::vector<int>
        lgr_index_map; /* A vector that maps LGR-nr to index into the LGR_list.*/
    int lgr_nr; /* The lgr_nr of the cell holding this nnc_info structure. */
};

static void nnc_info_add_vector(nnc_info_type *nnc_info,
                                nnc_vector_ptr &&nnc_vector) {
    int lgr_nr = nnc_vector_get_lgr_nr(nnc_vector.get());
    if (lgr_nr < 0)
        throw std::logic_error("lgr_nr was negative");
    nnc_info->lgr_list.push_back(std::move(nnc_vector));
    size_t lgr_index = static_cast<size_t>(lgr_nr);
    if (lgr_index >= nnc_info->lgr_index_map.size())
        nnc_info->lgr_index_map.resize(lgr_index + 1, -1);
    nnc_info->lgr_index_map.at(lgr_index) =
        static_cast<int>(nnc_info->lgr_list.size()) - 1;
}

nnc_info_type *nnc_info_alloc(int lgr_nr) {
    auto nnc_info = std::make_unique<nnc_info_type>();
    nnc_info->lgr_nr = lgr_nr;
    return nnc_info.release();
}

nnc_info_type *nnc_info_alloc_copy(const nnc_info_type *src_info) {
    nnc_info_type *copy_info = nnc_info_alloc(src_info->lgr_nr);

    for (const auto &vec : src_info->lgr_list) {
        nnc_vector_ptr copy_vector{nnc_vector_alloc_copy(vec.get()),
                                   nnc_vector_free};
        nnc_info_add_vector(copy_info, std::move(copy_vector));
    }

    return copy_info;
}

bool nnc_info_equal(const nnc_info_type *nnc_info1,
                    const nnc_info_type *nnc_info2) {
    if (nnc_info1 == nnc_info2)
        return true;

    if ((nnc_info1 == NULL) || (nnc_info2 == NULL))
        return false;

    {
        if (nnc_info1->lgr_nr != nnc_info2->lgr_nr)
            return false;

        if ((nnc_info1->lgr_index_map.size() > 0) &&
            (nnc_info2->lgr_index_map.size() > 0)) {
            size_t max_lgr_nr = std::max(nnc_info1->lgr_index_map.size(),
                                         nnc_info2->lgr_index_map.size());
            size_t lgr_nr = 0;

            while (true) {
                const nnc_vector_type *vector1 =
                    nnc_info_get_vector(nnc_info1, static_cast<int>(lgr_nr));
                const nnc_vector_type *vector2 =
                    nnc_info_get_vector(nnc_info2, static_cast<int>(lgr_nr));

                if (!nnc_vector_equal(vector1, vector2))
                    return false;

                lgr_nr++;
                if (lgr_nr >= max_lgr_nr)
                    return true;
            }
        } else {
            if (nnc_info1->lgr_index_map.size() ==
                nnc_info2->lgr_index_map.size())
                return true;
            else
                return false;
        }
    }
}

void nnc_info_free(nnc_info_type *nnc_info) { delete nnc_info; }

nnc_vector_type *nnc_info_get_vector(const nnc_info_type *nnc_info,
                                     int lgr_nr) {
    if (lgr_nr < 0 ||
        static_cast<size_t>(lgr_nr) >= nnc_info->lgr_index_map.size())
        return NULL;

    int lgr_index = nnc_info->lgr_index_map[lgr_nr];
    if (-1 == lgr_index)
        return NULL;
    else
        return nnc_info->lgr_list[lgr_index].get();
}

nnc_vector_type *nnc_info_iget_vector(const nnc_info_type *nnc_info,
                                      int lgr_index) {
    return nnc_info->lgr_list.at(lgr_index).get();
}

nnc_vector_type *nnc_info_get_self_vector(const nnc_info_type *nnc_info) {
    return nnc_info_get_vector(nnc_info, nnc_info->lgr_nr);
}

static void nnc_info_assert_vector(nnc_info_type *nnc_info, int lgr_nr) {
    nnc_vector_type *nnc_vector = nnc_info_get_vector(nnc_info, lgr_nr);
    if (!nnc_vector) {
        nnc_info_add_vector(nnc_info, make_nnc_vector(lgr_nr));
    }
}

void nnc_info_add_nnc(nnc_info_type *nnc_info, int lgr_nr,
                      int global_cell_number, int nnc_index) {
    nnc_info_assert_vector(nnc_info, lgr_nr);
    {
        nnc_vector_type *nnc_vector = nnc_info_get_vector(nnc_info, lgr_nr);
        nnc_vector_add_nnc(nnc_vector, global_cell_number, nnc_index);
    }
}

bool nnc_info_has_grid_index_list(const nnc_info_type *nnc_info, int lgr_nr) {
    return nnc_info_get_vector(nnc_info, lgr_nr);
}

const std::vector<int> &
nnc_info_get_grid_index_list(const nnc_info_type *nnc_info, int lgr_nr) {
    nnc_vector_type *nnc_vector = nnc_info_get_vector(nnc_info, lgr_nr);
    if (!nnc_vector)
        throw std::invalid_argument(std::string(__func__));
    return nnc_vector_get_grid_index_list(nnc_vector);
}

const std::vector<int> &
nnc_info_get_self_grid_index_list(const nnc_info_type *nnc_info) {
    return nnc_info_get_grid_index_list(nnc_info, nnc_info->lgr_nr);
}

int nnc_info_get_lgr_nr(const nnc_info_type *nnc_info) {
    return nnc_info->lgr_nr;
}

size_t nnc_info_get_size(const nnc_info_type *nnc_info) {
    return nnc_info->lgr_list.size();
}

int nnc_info_get_total_size(const nnc_info_type *nnc_info) {
    int num_nnc = 0;
    for (const auto &nnc_vector : nnc_info->lgr_list) {
        num_nnc += nnc_vector_get_size(nnc_vector.get());
    }
    return num_nnc;
}
