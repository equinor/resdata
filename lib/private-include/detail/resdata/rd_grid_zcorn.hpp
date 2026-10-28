#pragma once

#include <cstddef>
#include <stdexcept>

namespace rd {

/**
  Calculates the index of a corner in the zcorn array. The corner is given
  with i,j,k indices of the cell, and corner number [0...8). Mainly a
  convenience function for testing, and should not be used in a tight inner
  loop unrolling the zcorn vector.
*/
inline size_t grid_zcorn_index(size_t nx, size_t ny, size_t i, size_t j,
                               size_t k, size_t c) {
    if (c >= 8)
        throw std::invalid_argument(
            "grid_zcorn_index: corner must be in [0,8)");

    size_t zcorn_index = k * 8 * nx * ny + j * 4 * nx + 2 * i;
    if ((c % 2) == 1)
        zcorn_index += 1;

    if ((c % 4) == 2)
        zcorn_index += 2 * nx;

    if ((c % 4) == 3)
        zcorn_index += 2 * nx;

    if (c >= 4)
        zcorn_index += 4 * nx * ny;

    return zcorn_index;
}

} // namespace rd
