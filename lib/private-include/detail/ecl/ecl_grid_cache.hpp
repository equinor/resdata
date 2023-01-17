#ifndef ERT_ECL_GRID_CACHE_H
#define ERT_ECL_GRID_CACHE_H

#include <vector>

#include <ert/ecl/ecl_grid.hpp>

namespace ecl {
class ecl_grid_cache {
public:
    ecl_grid_cache(const ecl_grid_type *grid);

    const std::vector<double> &volume() const;
    const std::vector<double> &xpos() const { return this->xp; }
    const std::vector<double> &ypos() const { return this->yp; }
    const std::vector<double> &zpos() const { return this->zp; }
    const std::vector<int> &global_index() const { return this->gi; }
    int size() const { return this->xp.size(); }

private:
    const ecl_grid_type *grid;
    std::vector<int> gi;
    std::vector<double> xp;
    std::vector<double> yp;
    std::vector<double> zp;
    mutable std::vector<double> v;
};
} // namespace ecl

#endif
