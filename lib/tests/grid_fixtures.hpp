#pragma once

/**
 * Shared test fixtures that build grids
 */

#include <vector>

#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>

inline void set_pillar(rd_kw_type *coord_kw, int pillar, float tx, float ty,
                       float tz, float bx, float by, float bz) {
    const int off = 6 * pillar;
    rd_kw_iset_float(coord_kw, off + 0, tx);
    rd_kw_iset_float(coord_kw, off + 1, ty);
    rd_kw_iset_float(coord_kw, off + 2, tz);
    rd_kw_iset_float(coord_kw, off + 3, bx);
    rd_kw_iset_float(coord_kw, off + 4, by);
    rd_kw_iset_float(coord_kw, off + 5, bz);
}

/**
 * Generates a grid, using the rd_grid_alloc_GRDECL_kw constructor,
 * with the x,y,z coordinates of the 0th corner of the ijkth cel is
 * i,j,k. In other words this is a grid where the corners are on the
 * whole numbers.
 *
 * Takes a vector of i,j,k,c,z tuples which changes the corner c of cell
 * at i,j,k z value.
 */
inline rd_grid_ptr generate_coordkw_grid(
    int num_x, int num_y, int num_z,
    const std::vector<std::tuple<int, int, int, int, double>> &z_vector) {
    auto coord_kw =
        make_rd_kw(COORD_KW, RD_GRID_COORD_SIZE(num_x, num_y), RD_FLOAT);
    auto zcorn_kw =
        make_rd_kw(ZCORN_KW, RD_GRID_ZCORN_SIZE(num_x, num_y, num_z), RD_FLOAT);

    for (int j = 0; j < num_y; j++) {
        for (int i = 0; i < num_x; i++) {
            set_pillar(coord_kw.get(), i + j * num_x, i, j, -1, i, j, -1);

            for (int k = 0; k < num_z; k++) {
                for (int c = 0; c < 4; c++) {
                    int zi1 = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
                    int zi2 =
                        rd_grid_zcorn_index__(num_x, num_y, i, j, k, c + 4);

                    double z1 = k;
                    double z2 = k + 1;

                    rd_kw_iset_float(zcorn_kw.get(), zi1, z1);
                    rd_kw_iset_float(zcorn_kw.get(), zi2, z2);
                }
            }
        }
    }

    for (const auto &[i, j, k, c, z] : z_vector) {
        auto index = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
        rd_kw_iset_float(zcorn_kw.get(), index, z);
    }

    return {rd_grid_alloc_GRDECL_kw(num_x, num_y, num_z, zcorn_kw.get(),
                                    coord_kw.get(), nullptr, nullptr),
            &rd_grid_free};
}
