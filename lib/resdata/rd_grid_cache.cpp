#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>

#include "detail/resdata/rd_grid_cache.hpp"

/**
   The rd_grid_cache_struct data structure internalizes the world
   position of all the active cells. This is just a minor
   simplification to speed up repeated calls to get the true world
   coordinates of a cell.
*/

namespace rd {
rd_grid_cache::rd_grid_cache(const rd_grid_type *grid) : grid(grid) {
    for (int active_index = 0;
         active_index < rd_grid_get_active_size(this->grid); active_index++) {
        double x, y, z;
        int global_index = rd_grid_get_global_index1A(this->grid, active_index);
        rd_grid_get_xyz1(this->grid, global_index, &x, &y, &z);

        this->gi.push_back(global_index);
        this->xp.push_back(x);
        this->yp.push_back(y);
        this->zp.push_back(z);
    }
}

const std::vector<double> &rd_grid_cache::volume() const {
    if (this->v.empty()) {
        for (int active_index = 0; active_index < this->size(); active_index++)
            this->v.push_back(
                rd_grid_get_cell_volume1A(this->grid, active_index));
    }
    return this->v;
}

} // namespace rd
