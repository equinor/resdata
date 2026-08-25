#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <algorithm>

#include <resdata/rd_box.hpp>
#include <resdata/rd_grid.hpp>

namespace rd {
rd_box::rd_box(const rd_grid_type *grid, int i1, int i2, int j1, int j2, int k1,
               int k2)
    : grid(grid), i1(std::min(i1, i2)), i2(std::max(i1, i2)),
      j1(std::min(j1, j2)), j2(std::max(j1, j2)), k1(std::min(k1, k2)),
      k2(std::max(k1, k2)) {
    for (int k = this->k1; k <= this->k2; k++)
        for (int j = this->j1; j <= this->j2; j++)
            for (int i = this->i1; i <= this->i2; i++)
                this->global_index_list.push_back(
                    rd_grid_get_global_index3(this->grid, i, j, k));
}

const std::vector<int> &rd_box::global_list() const {
    return this->global_index_list;
}

} // namespace rd
