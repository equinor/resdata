#ifndef ERT_ECL_BOX_H
#define ERT_ECL_BOX_H

#include <vector>
#include <ert/ecl/ecl_grid.hpp>

namespace ecl {

class ecl_box {
public:
    ecl_box(const ecl_grid_type *grid, int i1, int i2, int j1, int j2, int k1,
            int k2);
    const std::vector<int> &active_list() const;

private:
    const ecl_grid_type *grid;

    int i1, i2, j1, j2, k1, k2;
    std::vector<int> active_index_list;
    std::vector<int> global_index_list;
};

} // namespace ecl
#endif
