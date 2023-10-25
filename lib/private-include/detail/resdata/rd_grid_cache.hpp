#ifndef ERT_RD_GRID_CACHE_H
#define ERT_RD_GRID_CACHE_H

#include <vector>

#include <resdata/rd_grid.hpp>

namespace rd {
class rd_grid_cache {
public:
    rd_grid_cache(const rd_grid_type *grid);

    const std::vector<double> &volume() const;
    const std::vector<double> &xpos() const { return this->xp; }
    const std::vector<double> &ypos() const { return this->yp; }
    const std::vector<double> &zpos() const { return this->zp; }
    const std::vector<int> &global_index() const { return this->gi; }
    int size() const { return this->xp.size(); }

private:
    const rd_grid_type *grid;
    std::vector<int> gi;
    std::vector<double> xp;
    std::vector<double> yp;
    std::vector<double> zp;
    mutable std::vector<double> v;
};
} // namespace rd

#endif
