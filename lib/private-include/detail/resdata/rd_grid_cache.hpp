#pragma once
#include <cstddef>
#include <vector>

#include <resdata/rd_grid.hpp>

namespace rd {
class grid_cache {
public:
    grid_cache(rd_grid_type *grid);

    const std::vector<double> &volume() const;
    const std::vector<double> &xpos() const { return this->xp; }
    const std::vector<double> &ypos() const { return this->yp; }
    const std::vector<double> &zpos() const { return this->zp; }
    const std::vector<size_t> &global_index() const { return this->gi; }
    size_t size() const { return this->xp.size(); }

private:
    rd_grid_type *grid;
    std::vector<size_t> gi;
    std::vector<double> xp;
    std::vector<double> yp;
    std::vector<double> zp;
    mutable std::vector<double> v;
};
} // namespace rd
