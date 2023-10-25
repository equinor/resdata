#ifndef ERT_RD_BOX_H
#define ERT_RD_BOX_H

#include <vector>
#include <resdata/rd_grid.hpp>

namespace rd {

class rd_box {
public:
    rd_box(const rd_grid_type *grid, int i1, int i2, int j1, int j2, int k1,
           int k2);
    const std::vector<int> &active_list() const;

private:
    const rd_grid_type *grid;

    int i1, i2, j1, j2, k1, k2;
    std::vector<int> active_index_list;
    std::vector<int> global_index_list;
};

} // namespace rd
#endif
