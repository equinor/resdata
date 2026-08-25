#ifndef ERT_RD_BOX_H
#define ERT_RD_BOX_H

#include <vector>
#include <resdata/rd_grid.hpp>

namespace rd {

class rd_box {
public:
    rd_box(const rd_grid_type *grid, size_t i1, size_t i2, size_t j1, size_t j2,
           size_t k1, size_t k2);
    /** Global indices of every cell in the box, active or not. */
    const std::vector<size_t> &global_list() const;

private:
    const rd_grid_type *grid;

    size_t i1, i2, j1, j2, k1, k2;
    std::vector<size_t> global_index_list;
};

} // namespace rd
#endif
