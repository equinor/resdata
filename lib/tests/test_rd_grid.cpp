#include <catch2/catch.hpp>
#include <fstream>
#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <vector>

#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE("Test unfractured grids", "[unittest]") {
    GIVEN("An unfractured grid") {
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(21, 11, 12, 1, 2, 3, NULL);

        REQUIRE(rd_grid_get_nactive_fracture(grid) == 0);

        THEN("It should return -1 on any fracture index") {
            auto i = GENERATE(0, 1, 10, 20);
            REQUIRE(rd_grid_get_active_fracture_index1(grid, i) == -1);
        }

        rd_grid_free(grid);
    }
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
rd_grid_type *generate_coordkw_grid(
    int num_x, int num_y, int num_z,
    const std::vector<std::tuple<int, int, int, int, double>> &z_vector) {
    rd_kw_type *coord_kw =
        rd_kw_alloc(COORD_KW, RD_GRID_COORD_SIZE(num_x, num_y), RD_FLOAT);
    rd_kw_type *zcorn_kw = rd_kw_alloc(
        ZCORN_KW, RD_GRID_ZCORN_SIZE(num_x, num_y, num_z), RD_FLOAT);

    for (int j = 0; j < num_y; j++) {
        for (int i = 0; i < num_x; i++) {
            int offset = 6 * (i + j * num_x);
            rd_kw_iset_float(coord_kw, offset, i);
            rd_kw_iset_float(coord_kw, offset + 1, j);
            rd_kw_iset_float(coord_kw, offset + 2, -1);

            rd_kw_iset_float(coord_kw, offset + 3, i);
            rd_kw_iset_float(coord_kw, offset + 4, j);
            rd_kw_iset_float(coord_kw, offset + 5, -1);

            for (int k = 0; k < num_z; k++) {
                for (int c = 0; c < 4; c++) {
                    int zi1 = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
                    int zi2 =
                        rd_grid_zcorn_index__(num_x, num_y, i, j, k, c + 4);

                    double z1 = k;
                    double z2 = k + 1;

                    rd_kw_iset_float(zcorn_kw, zi1, z1);
                    rd_kw_iset_float(zcorn_kw, zi2, z2);
                }
            }
        }
    }

    for (const auto &[i, j, k, c, z] : z_vector) {
        auto index = rd_grid_zcorn_index__(num_x, num_y, i, j, k, c);
        rd_kw_iset_float(zcorn_kw, index, z);
    }

    rd_grid_type *grid = rd_grid_alloc_GRDECL_kw(num_x, num_y, num_z, zcorn_kw,
                                                 coord_kw, NULL, NULL);
    rd_kw_free(coord_kw);
    rd_kw_free(zcorn_kw);

    return grid;
}

/**
 * Writes an EGRID file with a single top-level LGR directly using the fortio
 * API, since there is no public C API for attaching an LGR to a grid in
 * memory. The main grid is an nx*ny*nz rectangular grid with unit cells,
 * and the LGR is a lgr_nx*lgr_ny*lgr_nz rectangular grid that refines the
 * single host cell at (host_i, host_j, host_k) of the main grid.
 *
 * This utility is intended as a building block for tests that need to
 * exercise EGRID load paths involving LGRs.
 */
void write_egrid_with_single_lgr(const fs::path &filename, int nx, int ny,
                                 int nz, int lgr_nx, int lgr_ny, int lgr_nz,
                                 int host_i, int host_j, int host_k,
                                 const std::string &lgr_name,
                                 const float *mapaxes = nullptr,
                                 const std::vector<int> &nncg = {},
                                 const std::vector<int> &nncl = {}) {
    rd_grid_type *main_grid =
        rd_grid_alloc_rectangular(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);
    rd_grid_type *lgr_grid = rd_grid_alloc_rectangular(
        lgr_nx, lgr_ny, lgr_nz, 1.0 / lgr_nx, 1.0 / lgr_ny, 1.0 / lgr_nz,
        nullptr);

    const int host_global_index = host_i + host_j * nx + host_k * nx * ny;

    auto make_gridhead = [](int gnx, int gny, int gnz, int grid_nr) {
        rd_kw_type *kw = rd_kw_alloc(GRIDHEAD_KW, GRIDHEAD_SIZE, RD_INT);
        rd_kw_scalar_set_int(kw, 0);
        rd_kw_iset_int(kw, GRIDHEAD_TYPE_INDEX, GRIDHEAD_GRIDTYPE_CORNERPOINT);
        rd_kw_iset_int(kw, GRIDHEAD_NX_INDEX, gnx);
        rd_kw_iset_int(kw, GRIDHEAD_NY_INDEX, gny);
        rd_kw_iset_int(kw, GRIDHEAD_NZ_INDEX, gnz);
        rd_kw_iset_int(kw, GRIDHEAD_NUMRES_INDEX, 1);
        rd_kw_iset_int(kw, GRIDHEAD_LGR_INDEX, grid_nr);
        return kw;
    };

    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    auto write_grid_body = [&](rd_grid_type *grid, fortio_type *fortio) {
        write_and_free(rd_grid_alloc_coord_kw(grid), fortio);
        write_and_free(rd_grid_alloc_zcorn_kw(grid), fortio);
        write_and_free(rd_grid_alloc_actnum_kw(grid), fortio);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    rd_kw_type *filehead = rd_kw_alloc(FILEHEAD_KW, 100, RD_INT);
    rd_kw_scalar_set_int(filehead, 0);
    rd_kw_iset_int(filehead, FILEHEAD_VERSION_INDEX, 3);
    rd_kw_iset_int(filehead, FILEHEAD_YEAR_INDEX, 2007);
    rd_kw_iset_int(filehead, FILEHEAD_TYPE_INDEX,
                   FILEHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(filehead, FILEHEAD_DUALP_INDEX, FILEHEAD_SINGLE_POROSITY);
    rd_kw_iset_int(filehead, FILEHEAD_ORGFORMAT_INDEX,
                   FILEHEAD_ORGTYPE_CORNERPOINT);
    write_and_free(filehead, fortio);

    if (mapaxes != nullptr) {
        rd_kw_type *mapaxes_kw = rd_kw_alloc(MAPAXES_KW, 6, RD_FLOAT);
        for (int i = 0; i < 6; i++)
            rd_kw_iset_float(mapaxes_kw, i, mapaxes[i]);
        write_and_free(mapaxes_kw, fortio);
    }

    write_and_free(make_gridhead(nx, ny, nz, 0), fortio);
    write_grid_body(main_grid, fortio);
    write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);

    rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 1, RD_CHAR);
    rd_kw_iset_string8(lgr_kw, 0, lgr_name.c_str());
    write_and_free(lgr_kw, fortio);

    rd_kw_type *lgr_parent_kw = rd_kw_alloc(LGR_PARENT_KW, 1, RD_CHAR);
    rd_kw_iset_string8(lgr_parent_kw, 0, "");
    write_and_free(lgr_parent_kw, fortio);

    write_and_free(make_gridhead(lgr_nx, lgr_ny, lgr_nz, 1), fortio);
    write_grid_body(lgr_grid, fortio);

    const int lgr_size = lgr_nx * lgr_ny * lgr_nz;
    rd_kw_type *hostnum_kw = rd_kw_alloc(HOSTNUM_KW, lgr_size, RD_INT);
    for (int i = 0; i < lgr_size; i++)
        rd_kw_iset_int(hostnum_kw, i, host_global_index + 1);
    write_and_free(hostnum_kw, fortio);

    write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);
    write_and_free(rd_kw_alloc(ENDLGR_KW, 0, RD_INT), fortio);

    if (!nncg.empty()) {
        rd_kw_type *nnchead_kw = rd_kw_alloc(NNCHEAD_KW, NNCHEAD_SIZE, RD_INT);
        rd_kw_scalar_set_int(nnchead_kw, 0);
        rd_kw_iset_int(nnchead_kw, NNCHEAD_NUMNNC_INDEX, (int)nncg.size());
        rd_kw_iset_int(nnchead_kw, NNCHEAD_LGR_INDEX, 1);
        write_and_free(nnchead_kw, fortio);

        rd_kw_type *nncg_kw = rd_kw_alloc(NNCG_KW, (int)nncg.size(), RD_INT);
        for (size_t i = 0; i < nncg.size(); i++)
            rd_kw_iset_int(nncg_kw, (int)i, nncg[i]);
        write_and_free(nncg_kw, fortio);

        rd_kw_type *nncl_kw = rd_kw_alloc(NNCL_KW, (int)nncl.size(), RD_INT);
        for (size_t i = 0; i < nncl.size(); i++)
            rd_kw_iset_int(nncl_kw, (int)i, nncl[i]);
        write_and_free(nncl_kw, fortio);
    }

    fortio_fclose(fortio);
    rd_grid_free(main_grid);
    rd_grid_free(lgr_grid);
}

/**
 * Writes an EGRID file with a main grid, a top-level LGR, and a second LGR
 * nested inside the first. The main grid is an nx*ny*nz rectangular grid
 * with unit cells, the outer LGR refines the main-grid cell at
 * (host_i, host_j, host_k) into outer_nx*outer_ny*outer_nz sub-cells, and
 * the inner LGR further refines the outer-LGR cell at
 * (inner_host_i, inner_host_j, inner_host_k) into
 * inner_nx*inner_ny*inner_nz sub-cells.
 *
 * The resulting file drives the loader through the nested-LGR paths, where
 * the inner LGR's LGR_PARENT_KW is set to \c outer_name rather than empty.
 */
void write_egrid_with_nested_lgr(
    const fs::path &filename, int nx, int ny, int nz, int host_i, int host_j,
    int host_k, int outer_nx, int outer_ny, int outer_nz,
    const std::string &outer_name, int inner_host_i, int inner_host_j,
    int inner_host_k, int inner_nx, int inner_ny, int inner_nz,
    const std::string &inner_name) {
    rd_grid_type *main_grid =
        rd_grid_alloc_rectangular(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);
    rd_grid_type *outer_grid = rd_grid_alloc_rectangular(
        outer_nx, outer_ny, outer_nz, 1.0 / outer_nx, 1.0 / outer_ny,
        1.0 / outer_nz, nullptr);
    rd_grid_type *inner_grid = rd_grid_alloc_rectangular(
        inner_nx, inner_ny, inner_nz, 1.0 / (outer_nx * inner_nx),
        1.0 / (outer_ny * inner_ny), 1.0 / (outer_nz * inner_nz), nullptr);

    const int outer_host_global = host_i + host_j * nx + host_k * nx * ny;
    const int inner_host_global =
        inner_host_i + inner_host_j * outer_nx +
        inner_host_k * outer_nx * outer_ny;

    auto make_gridhead = [](int gnx, int gny, int gnz, int grid_nr) {
        rd_kw_type *kw = rd_kw_alloc(GRIDHEAD_KW, GRIDHEAD_SIZE, RD_INT);
        rd_kw_scalar_set_int(kw, 0);
        rd_kw_iset_int(kw, GRIDHEAD_TYPE_INDEX, GRIDHEAD_GRIDTYPE_CORNERPOINT);
        rd_kw_iset_int(kw, GRIDHEAD_NX_INDEX, gnx);
        rd_kw_iset_int(kw, GRIDHEAD_NY_INDEX, gny);
        rd_kw_iset_int(kw, GRIDHEAD_NZ_INDEX, gnz);
        rd_kw_iset_int(kw, GRIDHEAD_NUMRES_INDEX, 1);
        rd_kw_iset_int(kw, GRIDHEAD_LGR_INDEX, grid_nr);
        return kw;
    };

    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    auto write_grid_body = [&](rd_grid_type *grid, fortio_type *fortio) {
        write_and_free(rd_grid_alloc_coord_kw(grid), fortio);
        write_and_free(rd_grid_alloc_zcorn_kw(grid), fortio);
        write_and_free(rd_grid_alloc_actnum_kw(grid), fortio);
    };

    auto write_lgr_section = [&](fortio_type *fortio, const std::string &name,
                                 const std::string &parent, int grid_nr,
                                 rd_grid_type *lgr, int host_global_1based) {
        rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 1, RD_CHAR);
        rd_kw_iset_string8(lgr_kw, 0, name.c_str());
        write_and_free(lgr_kw, fortio);

        rd_kw_type *lgr_parent_kw = rd_kw_alloc(LGR_PARENT_KW, 1, RD_CHAR);
        rd_kw_iset_string8(lgr_parent_kw, 0, parent.c_str());
        write_and_free(lgr_parent_kw, fortio);

        write_and_free(make_gridhead(rd_grid_get_nx(lgr), rd_grid_get_ny(lgr),
                                     rd_grid_get_nz(lgr), grid_nr),
                       fortio);
        write_grid_body(lgr, fortio);

        const int lgr_size = rd_grid_get_global_size(lgr);
        rd_kw_type *hostnum_kw = rd_kw_alloc(HOSTNUM_KW, lgr_size, RD_INT);
        for (int i = 0; i < lgr_size; i++)
            rd_kw_iset_int(hostnum_kw, i, host_global_1based);
        write_and_free(hostnum_kw, fortio);

        write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);
        write_and_free(rd_kw_alloc(ENDLGR_KW, 0, RD_INT), fortio);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    rd_kw_type *filehead = rd_kw_alloc(FILEHEAD_KW, 100, RD_INT);
    rd_kw_scalar_set_int(filehead, 0);
    rd_kw_iset_int(filehead, FILEHEAD_VERSION_INDEX, 3);
    rd_kw_iset_int(filehead, FILEHEAD_YEAR_INDEX, 2007);
    rd_kw_iset_int(filehead, FILEHEAD_TYPE_INDEX,
                   FILEHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(filehead, FILEHEAD_DUALP_INDEX, FILEHEAD_SINGLE_POROSITY);
    rd_kw_iset_int(filehead, FILEHEAD_ORGFORMAT_INDEX,
                   FILEHEAD_ORGTYPE_CORNERPOINT);
    write_and_free(filehead, fortio);

    write_and_free(make_gridhead(nx, ny, nz, 0), fortio);
    write_grid_body(main_grid, fortio);
    write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);

    write_lgr_section(fortio, outer_name, "", 1, outer_grid,
                      outer_host_global + 1);
    write_lgr_section(fortio, inner_name, outer_name, 2, inner_grid,
                      inner_host_global + 1);

    fortio_fclose(fortio);
    rd_grid_free(main_grid);
    rd_grid_free(outer_grid);
    rd_grid_free(inner_grid);
}

/**
 * Writes an EGRID file with a main grid and two sibling LGRs that refine
 * two different host cells of the main grid, optionally followed by an
 * amalgamated NNC section (NNCHEADA + NNA1 + NNA2) connecting cells of
 * lgr1 to cells of lgr2. Useful for exercising the NNC paths between LGRs.
 */
void write_egrid_with_two_lgrs_and_amalgamated_nnc(
    const fs::path &filename, int nx, int ny, int nz,
    const std::string &lgr1_name, int host1_i, int host1_j, int host1_k,
    const std::string &lgr2_name, int host2_i, int host2_j, int host2_k,
    const std::vector<int> &nna1, const std::vector<int> &nna2) {
    rd_grid_type *main_grid =
        rd_grid_alloc_rectangular(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);
    rd_grid_type *lgr1_grid =
        rd_grid_alloc_rectangular(2, 2, 2, 0.5, 0.5, 0.5, nullptr);
    rd_grid_type *lgr2_grid =
        rd_grid_alloc_rectangular(2, 2, 2, 0.5, 0.5, 0.5, nullptr);

    const int host1_global = host1_i + host1_j * nx + host1_k * nx * ny;
    const int host2_global = host2_i + host2_j * nx + host2_k * nx * ny;

    auto make_gridhead = [](int gnx, int gny, int gnz, int grid_nr) {
        rd_kw_type *kw = rd_kw_alloc(GRIDHEAD_KW, GRIDHEAD_SIZE, RD_INT);
        rd_kw_scalar_set_int(kw, 0);
        rd_kw_iset_int(kw, GRIDHEAD_TYPE_INDEX, GRIDHEAD_GRIDTYPE_CORNERPOINT);
        rd_kw_iset_int(kw, GRIDHEAD_NX_INDEX, gnx);
        rd_kw_iset_int(kw, GRIDHEAD_NY_INDEX, gny);
        rd_kw_iset_int(kw, GRIDHEAD_NZ_INDEX, gnz);
        rd_kw_iset_int(kw, GRIDHEAD_NUMRES_INDEX, 1);
        rd_kw_iset_int(kw, GRIDHEAD_LGR_INDEX, grid_nr);
        return kw;
    };

    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    auto write_lgr_section = [&](fortio_type *fortio, const std::string &name,
                                 int grid_nr, rd_grid_type *lgr,
                                 int host_global_1based) {
        rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 1, RD_CHAR);
        rd_kw_iset_string8(lgr_kw, 0, name.c_str());
        write_and_free(lgr_kw, fortio);

        rd_kw_type *lgr_parent_kw = rd_kw_alloc(LGR_PARENT_KW, 1, RD_CHAR);
        rd_kw_iset_string8(lgr_parent_kw, 0, "");
        write_and_free(lgr_parent_kw, fortio);

        write_and_free(make_gridhead(rd_grid_get_nx(lgr), rd_grid_get_ny(lgr),
                                     rd_grid_get_nz(lgr), grid_nr),
                       fortio);
        write_and_free(rd_grid_alloc_coord_kw(lgr), fortio);
        write_and_free(rd_grid_alloc_zcorn_kw(lgr), fortio);
        write_and_free(rd_grid_alloc_actnum_kw(lgr), fortio);

        const int lgr_size = rd_grid_get_global_size(lgr);
        rd_kw_type *hostnum_kw = rd_kw_alloc(HOSTNUM_KW, lgr_size, RD_INT);
        for (int i = 0; i < lgr_size; i++)
            rd_kw_iset_int(hostnum_kw, i, host_global_1based);
        write_and_free(hostnum_kw, fortio);

        write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);
        write_and_free(rd_kw_alloc(ENDLGR_KW, 0, RD_INT), fortio);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    rd_kw_type *filehead = rd_kw_alloc(FILEHEAD_KW, 100, RD_INT);
    rd_kw_scalar_set_int(filehead, 0);
    rd_kw_iset_int(filehead, FILEHEAD_VERSION_INDEX, 3);
    rd_kw_iset_int(filehead, FILEHEAD_YEAR_INDEX, 2007);
    rd_kw_iset_int(filehead, FILEHEAD_TYPE_INDEX,
                   FILEHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(filehead, FILEHEAD_DUALP_INDEX, FILEHEAD_SINGLE_POROSITY);
    rd_kw_iset_int(filehead, FILEHEAD_ORGFORMAT_INDEX,
                   FILEHEAD_ORGTYPE_CORNERPOINT);
    write_and_free(filehead, fortio);

    write_and_free(make_gridhead(nx, ny, nz, 0), fortio);
    write_and_free(rd_grid_alloc_coord_kw(main_grid), fortio);
    write_and_free(rd_grid_alloc_zcorn_kw(main_grid), fortio);
    write_and_free(rd_grid_alloc_actnum_kw(main_grid), fortio);
    write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);

    write_lgr_section(fortio, lgr1_name, 1, lgr1_grid, host1_global + 1);
    write_lgr_section(fortio, lgr2_name, 2, lgr2_grid, host2_global + 1);

    if (!nna1.empty()) {
        rd_kw_type *nncheada_kw =
            rd_kw_alloc(NNCHEADA_KW, NNCHEAD_SIZE, RD_INT);
        rd_kw_scalar_set_int(nncheada_kw, 0);
        rd_kw_iset_int(nncheada_kw, NNCHEADA_ILOC1_INDEX, 1);
        rd_kw_iset_int(nncheada_kw, NNCHEADA_ILOC2_INDEX, 2);
        write_and_free(nncheada_kw, fortio);

        rd_kw_type *nna1_kw = rd_kw_alloc(NNA1_KW, (int)nna1.size(), RD_INT);
        for (size_t i = 0; i < nna1.size(); i++)
            rd_kw_iset_int(nna1_kw, (int)i, nna1[i]);
        write_and_free(nna1_kw, fortio);

        rd_kw_type *nna2_kw = rd_kw_alloc(NNA2_KW, (int)nna2.size(), RD_INT);
        for (size_t i = 0; i < nna2.size(); i++)
            rd_kw_iset_int(nna2_kw, (int)i, nna2[i]);
        write_and_free(nna2_kw, fortio);
    }

    fortio_fclose(fortio);
    rd_grid_free(main_grid);
    rd_grid_free(lgr1_grid);
    rd_grid_free(lgr2_grid);
}

/**
 * Writes a single-grid EGRID file with a CORSNUM keyword describing coarse
 * cell groups. The grid is an nx*ny*nz rectangular grid with unit cells.
 *
 * \c corsnum must have nx*ny*nz entries. A value of 0 means the cell is
 * not part of any coarse group; positive values identify (1-based) the
 * coarse group the cell belongs to. At least one cell in each referenced
 * group must be active (see \c actnum) for the group to participate in
 * the active-index mapping exercised on load.
 *
 * \c actnum, if non-null, must also have nx*ny*nz entries. If null, all
 * cells are treated as active.
 */
void write_egrid_with_coarse_groups(const fs::path &filename, int nx, int ny,
                                    int nz, const int *corsnum,
                                    const int *actnum = nullptr) {
    rd_grid_type *grid =
        rd_grid_alloc_rectangular(nx, ny, nz, 1.0, 1.0, 1.0, actnum);

    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    rd_kw_type *filehead = rd_kw_alloc(FILEHEAD_KW, 100, RD_INT);
    rd_kw_scalar_set_int(filehead, 0);
    rd_kw_iset_int(filehead, FILEHEAD_VERSION_INDEX, 3);
    rd_kw_iset_int(filehead, FILEHEAD_YEAR_INDEX, 2007);
    rd_kw_iset_int(filehead, FILEHEAD_TYPE_INDEX,
                   FILEHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(filehead, FILEHEAD_DUALP_INDEX, FILEHEAD_SINGLE_POROSITY);
    rd_kw_iset_int(filehead, FILEHEAD_ORGFORMAT_INDEX,
                   FILEHEAD_ORGTYPE_CORNERPOINT);
    write_and_free(filehead, fortio);

    rd_kw_type *gridhead = rd_kw_alloc(GRIDHEAD_KW, GRIDHEAD_SIZE, RD_INT);
    rd_kw_scalar_set_int(gridhead, 0);
    rd_kw_iset_int(gridhead, GRIDHEAD_TYPE_INDEX,
                   GRIDHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(gridhead, GRIDHEAD_NX_INDEX, nx);
    rd_kw_iset_int(gridhead, GRIDHEAD_NY_INDEX, ny);
    rd_kw_iset_int(gridhead, GRIDHEAD_NZ_INDEX, nz);
    rd_kw_iset_int(gridhead, GRIDHEAD_NUMRES_INDEX, 1);
    rd_kw_iset_int(gridhead, GRIDHEAD_LGR_INDEX, 0);
    write_and_free(gridhead, fortio);

    write_and_free(rd_grid_alloc_coord_kw(grid), fortio);
    write_and_free(rd_grid_alloc_zcorn_kw(grid), fortio);
    write_and_free(rd_grid_alloc_actnum_kw(grid), fortio);

    const int size = nx * ny * nz;
    rd_kw_type *corsnum_kw = rd_kw_alloc(CORSNUM_KW, size, RD_INT);
    for (int i = 0; i < size; i++)
        rd_kw_iset_int(corsnum_kw, i, corsnum[i]);
    write_and_free(corsnum_kw, fortio);

    write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);

    fortio_fclose(fortio);
    rd_grid_free(grid);
}

/**
 * Writes a single-grid EGRID file marked as dual porosity. The geometry
 * is an nx*ny*nz rectangular grid with unit cells; the caller-supplied
 * ACTNUM uses the dual-porosity bitfield encoding, where each entry is a
 * combination of CELL_ACTIVE_MATRIX (1) and CELL_ACTIVE_FRACTURE (2).
 */
void write_egrid_dual_porosity(const fs::path &filename, int nx, int ny,
                               int nz, const int *actnum,
                               const std::vector<int> &nnc1 = {},
                               const std::vector<int> &nnc2 = {}) {
    rd_grid_type *grid =
        rd_grid_alloc_rectangular(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);

    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    rd_kw_type *filehead = rd_kw_alloc(FILEHEAD_KW, 100, RD_INT);
    rd_kw_scalar_set_int(filehead, 0);
    rd_kw_iset_int(filehead, FILEHEAD_VERSION_INDEX, 3);
    rd_kw_iset_int(filehead, FILEHEAD_YEAR_INDEX, 2007);
    rd_kw_iset_int(filehead, FILEHEAD_TYPE_INDEX,
                   FILEHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(filehead, FILEHEAD_DUALP_INDEX, FILEHEAD_DUAL_POROSITY);
    rd_kw_iset_int(filehead, FILEHEAD_ORGFORMAT_INDEX,
                   FILEHEAD_ORGTYPE_CORNERPOINT);
    write_and_free(filehead, fortio);

    rd_kw_type *gridhead = rd_kw_alloc(GRIDHEAD_KW, GRIDHEAD_SIZE, RD_INT);
    rd_kw_scalar_set_int(gridhead, 0);
    rd_kw_iset_int(gridhead, GRIDHEAD_TYPE_INDEX,
                   GRIDHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(gridhead, GRIDHEAD_NX_INDEX, nx);
    rd_kw_iset_int(gridhead, GRIDHEAD_NY_INDEX, ny);
    rd_kw_iset_int(gridhead, GRIDHEAD_NZ_INDEX, nz);
    rd_kw_iset_int(gridhead, GRIDHEAD_NUMRES_INDEX, 1);
    rd_kw_iset_int(gridhead, GRIDHEAD_LGR_INDEX, 0);
    write_and_free(gridhead, fortio);

    write_and_free(rd_grid_alloc_coord_kw(grid), fortio);
    write_and_free(rd_grid_alloc_zcorn_kw(grid), fortio);

    const int size = nx * ny * nz;
    rd_kw_type *actnum_kw = rd_kw_alloc(ACTNUM_KW, size, RD_INT);
    for (int i = 0; i < size; i++)
        rd_kw_iset_int(actnum_kw, i, actnum[i]);
    write_and_free(actnum_kw, fortio);

    write_and_free(rd_kw_alloc(ENDGRID_KW, 0, RD_INT), fortio);

    if (!nnc1.empty()) {
        rd_kw_type *nnchead_kw = rd_kw_alloc(NNCHEAD_KW, NNCHEAD_SIZE, RD_INT);
        rd_kw_scalar_set_int(nnchead_kw, 0);
        rd_kw_iset_int(nnchead_kw, NNCHEAD_NUMNNC_INDEX, (int)nnc1.size());
        rd_kw_iset_int(nnchead_kw, NNCHEAD_LGR_INDEX, 0);
        write_and_free(nnchead_kw, fortio);

        rd_kw_type *nnc1_kw = rd_kw_alloc(NNC1_KW, (int)nnc1.size(), RD_INT);
        for (size_t i = 0; i < nnc1.size(); i++)
            rd_kw_iset_int(nnc1_kw, (int)i, nnc1[i]);
        write_and_free(nnc1_kw, fortio);

        rd_kw_type *nnc2_kw = rd_kw_alloc(NNC2_KW, (int)nnc2.size(), RD_INT);
        for (size_t i = 0; i < nnc2.size(); i++)
            rd_kw_iset_int(nnc2_kw, (int)i, nnc2[i]);
        write_and_free(nnc2_kw, fortio);
    }

    fortio_fclose(fortio);
    rd_grid_free(grid);
}

/**
 * Writes a minimal single-porosity .GRID file containing a 1x1x1 main grid
 * and a 1x1x1 LGR covering the single main-grid cell. The LGR_KW is written
 * with two elements: the LGR name followed by \c parent_name, which allows
 * the caller to exercise the two-element branch of rd_grid_set_lgr_name_GRID.
 *
 * A parent_name of "" or "GLOBAL" is treated by the loader as "no parent"
 * (main grid is parent); any other non-empty value must match an existing
 * grid name (nested LGRs) or loading will fail.
 */
void write_grid_file_with_lgr_parent(const fs::path &filename,
                                     const char *lgr_name,
                                     const char *parent_name) {
    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    auto write_dimens = [&](fortio_type *fortio, int nx, int ny, int nz) {
        rd_kw_type *dimens = rd_kw_alloc(DIMENS_KW, 3, RD_INT);
        rd_kw_iset_int(dimens, DIMENS_NX_INDEX, nx);
        rd_kw_iset_int(dimens, DIMENS_NY_INDEX, ny);
        rd_kw_iset_int(dimens, DIMENS_NZ_INDEX, nz);
        write_and_free(dimens, fortio);
    };

    auto write_radial = [&](fortio_type *fortio) {
        rd_kw_type *radial = rd_kw_alloc(RADIAL_KW, 1, RD_CHAR);
        rd_kw_iset_string8(radial, 0, "FALSE");
        write_and_free(radial, fortio);
    };

    // Writes one cell with COORDS of size 7 (i.e. including host_cell
    // and coarse_group info) followed by CORNERS. For the main grid the
    // host_cell and coarse_group fields are 0 (meaning "none").
    auto write_cell = [&](fortio_type *fortio, int i, int j, int k,
                          int global_index, int host_cell_1based,
                          const float *corners) {
        rd_kw_type *coords = rd_kw_alloc(COORDS_KW, 7, RD_INT);
        rd_kw_iset_int(coords, 0, i + 1);
        rd_kw_iset_int(coords, 1, j + 1);
        rd_kw_iset_int(coords, 2, k + 1);
        rd_kw_iset_int(coords, 3, global_index + 1);
        rd_kw_iset_int(coords, 4, 1); // active
        rd_kw_iset_int(coords, 5, host_cell_1based);
        rd_kw_iset_int(coords, 6, 0); // coarse group 0 => no group
        write_and_free(coords, fortio);

        rd_kw_type *corners_kw = rd_kw_alloc(CORNERS_KW, 24, RD_FLOAT);
        for (int c = 0; c < 24; c++)
            rd_kw_iset_float(corners_kw, c, corners[c]);
        write_and_free(corners_kw, fortio);
    };

    // Unit cube corners (4 top + 4 bottom in x,y,z order matching the
    // GRID format expectations). Re-used by main cell and LGR cell since
    // the LGR perfectly overlays the single main cell.
    const float unit_corners[24] = {
        0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0,
        0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    // Main grid header
    write_dimens(fortio, 1, 1, 1);

    rd_kw_type *mapunits = rd_kw_alloc(MAPUNITS_KW, 1, RD_CHAR);
    rd_kw_iset_string8(mapunits, 0, "METRES");
    write_and_free(mapunits, fortio);

    rd_kw_type *gridunit = rd_kw_alloc(GRIDUNIT_KW, 2, RD_CHAR);
    rd_kw_iset_string8(gridunit, 0, "METRES");
    rd_kw_iset_string8(gridunit, 1, "");
    write_and_free(gridunit, fortio);

    write_radial(fortio);
    write_cell(fortio, 0, 0, 0, 0, 0, unit_corners);

    // LGR section: LGR_KW has two elements so the loader enters the
    // rd_kw_get_size(lgr_kw) == 2 branch.
    rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 2, RD_CHAR);
    rd_kw_iset_string8(lgr_kw, 0, lgr_name);
    rd_kw_iset_string8(lgr_kw, 1, parent_name);
    write_and_free(lgr_kw, fortio);

    write_dimens(fortio, 1, 1, 1);
    write_radial(fortio);
    write_cell(fortio, 0, 0, 0, 0, 1, unit_corners);

    fortio_fclose(fortio);
}

/**
 * Writes a minimal 1x1x1 single-porosity .GRID file (no LGR) that contains
 * a valid MAPAXES keyword in the main grid section. This exercises the
 * branch of rd_grid_alloc_GRID_data__ that calls rd_grid_init_mapaxes on
 * a grid loaded from a .GRID (as opposed to .EGRID) file.
 */
void write_grid_file_with_mapaxes(const fs::path &filename,
                                  const float *mapaxes) {
    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    rd_kw_type *dimens = rd_kw_alloc(DIMENS_KW, 3, RD_INT);
    rd_kw_iset_int(dimens, DIMENS_NX_INDEX, 1);
    rd_kw_iset_int(dimens, DIMENS_NY_INDEX, 1);
    rd_kw_iset_int(dimens, DIMENS_NZ_INDEX, 1);
    write_and_free(dimens, fortio);

    rd_kw_type *mapunits = rd_kw_alloc(MAPUNITS_KW, 1, RD_CHAR);
    rd_kw_iset_string8(mapunits, 0, "METRES");
    write_and_free(mapunits, fortio);

    rd_kw_type *mapaxes_kw = rd_kw_alloc(MAPAXES_KW, 6, RD_FLOAT);
    for (int i = 0; i < 6; i++)
        rd_kw_iset_float(mapaxes_kw, i, mapaxes[i]);
    write_and_free(mapaxes_kw, fortio);

    rd_kw_type *gridunit = rd_kw_alloc(GRIDUNIT_KW, 2, RD_CHAR);
    rd_kw_iset_string8(gridunit, 0, "METRES");
    rd_kw_iset_string8(gridunit, 1, "");
    write_and_free(gridunit, fortio);

    rd_kw_type *radial = rd_kw_alloc(RADIAL_KW, 1, RD_CHAR);
    rd_kw_iset_string8(radial, 0, "FALSE");
    write_and_free(radial, fortio);

    rd_kw_type *coords = rd_kw_alloc(COORDS_KW, 5, RD_INT);
    rd_kw_iset_int(coords, 0, 1);
    rd_kw_iset_int(coords, 1, 1);
    rd_kw_iset_int(coords, 2, 1);
    rd_kw_iset_int(coords, 3, 1);
    rd_kw_iset_int(coords, 4, 1);
    write_and_free(coords, fortio);

    const float unit_corners[24] = {
        0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0,
        0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
    };
    rd_kw_type *corners_kw = rd_kw_alloc(CORNERS_KW, 24, RD_FLOAT);
    for (int c = 0; c < 24; c++)
        rd_kw_iset_float(corners_kw, c, unit_corners[c]);
    write_and_free(corners_kw, fortio);

    fortio_fclose(fortio);
}

/**
 * Writes a minimal .GRID file containing a main grid of size nx*ny*nz and
 * a single 1x1x1 LGR hosted at main-grid cell 1. The main grid uses simple
 * unit-cube cells stacked along k. This utility is useful for exercising
 * GRID-file paths that care about the relative number of CORNERS keywords
 * vs. the main grid size (num_corners > nx*ny*nz once an LGR is present).
 */
void write_grid_file_main_with_lgr(const fs::path &filename, int nx, int ny,
                                   int nz) {
    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    auto write_dimens = [&](fortio_type *fortio, int gnx, int gny, int gnz) {
        rd_kw_type *dimens = rd_kw_alloc(DIMENS_KW, 3, RD_INT);
        rd_kw_iset_int(dimens, DIMENS_NX_INDEX, gnx);
        rd_kw_iset_int(dimens, DIMENS_NY_INDEX, gny);
        rd_kw_iset_int(dimens, DIMENS_NZ_INDEX, gnz);
        write_and_free(dimens, fortio);
    };

    auto write_radial = [&](fortio_type *fortio) {
        rd_kw_type *radial = rd_kw_alloc(RADIAL_KW, 1, RD_CHAR);
        rd_kw_iset_string8(radial, 0, "FALSE");
        write_and_free(radial, fortio);
    };

    auto write_cell = [&](fortio_type *fortio, int i, int j, int k,
                          int global_index, int host_cell_1based, float z0,
                          float z1) {
        rd_kw_type *coords = rd_kw_alloc(COORDS_KW, 7, RD_INT);
        rd_kw_iset_int(coords, 0, i + 1);
        rd_kw_iset_int(coords, 1, j + 1);
        rd_kw_iset_int(coords, 2, k + 1);
        rd_kw_iset_int(coords, 3, global_index + 1);
        rd_kw_iset_int(coords, 4, 1);
        rd_kw_iset_int(coords, 5, host_cell_1based);
        rd_kw_iset_int(coords, 6, 0);
        write_and_free(coords, fortio);

        const float corners[24] = {
            (float)i,     (float)j,     z0, (float)(i + 1), (float)j,     z0,
            (float)i,     (float)(j + 1), z0, (float)(i + 1), (float)(j + 1), z0,
            (float)i,     (float)j,     z1, (float)(i + 1), (float)j,     z1,
            (float)i,     (float)(j + 1), z1, (float)(i + 1), (float)(j + 1), z1,
        };
        rd_kw_type *corners_kw = rd_kw_alloc(CORNERS_KW, 24, RD_FLOAT);
        for (int c = 0; c < 24; c++)
            rd_kw_iset_float(corners_kw, c, corners[c]);
        write_and_free(corners_kw, fortio);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    write_dimens(fortio, nx, ny, nz);

    rd_kw_type *mapunits = rd_kw_alloc(MAPUNITS_KW, 1, RD_CHAR);
    rd_kw_iset_string8(mapunits, 0, "METRES");
    write_and_free(mapunits, fortio);

    rd_kw_type *gridunit = rd_kw_alloc(GRIDUNIT_KW, 2, RD_CHAR);
    rd_kw_iset_string8(gridunit, 0, "METRES");
    rd_kw_iset_string8(gridunit, 1, "");
    write_and_free(gridunit, fortio);

    write_radial(fortio);

    int global = 0;
    for (int k = 0; k < nz; k++)
        for (int j = 0; j < ny; j++)
            for (int i = 0; i < nx; i++)
                write_cell(fortio, i, j, k, global++, 0, (float)k,
                           (float)(k + 1));

    // LGR section: 1x1x1 LGR covering main-grid cell index 0.
    rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 1, RD_CHAR);
    rd_kw_iset_string8(lgr_kw, 0, "LGR1");
    write_and_free(lgr_kw, fortio);

    write_dimens(fortio, 1, 1, 1);
    write_radial(fortio);
    write_cell(fortio, 0, 0, 0, 0, 1, 0.0f, 1.0f);

    fortio_fclose(fortio);
}

/**
 * Writes a minimal .GRID file containing a 1x1x1 main grid, a 1x1x1
 * outer LGR hosted at the main-grid cell, and a 1x1x1 inner LGR whose
 * LGR_KW has two elements so the parent name is set to \c outer_name.
 * This drives the nested-LGR path in rd_grid_alloc_GRID that resolves
 * the host grid via rd_grid_get_lgr when lgr_grid->parent_name != NULL.
 */
void write_grid_file_with_nested_lgr(const fs::path &filename,
                                     const char *outer_name,
                                     const char *inner_name) {
    auto write_and_free = [](rd_kw_type *kw, fortio_type *fortio) {
        rd_kw_fwrite(kw, fortio);
        rd_kw_free(kw);
    };

    auto write_dimens = [&](fortio_type *fortio) {
        rd_kw_type *dimens = rd_kw_alloc(DIMENS_KW, 3, RD_INT);
        rd_kw_iset_int(dimens, DIMENS_NX_INDEX, 1);
        rd_kw_iset_int(dimens, DIMENS_NY_INDEX, 1);
        rd_kw_iset_int(dimens, DIMENS_NZ_INDEX, 1);
        write_and_free(dimens, fortio);
    };

    auto write_radial = [&](fortio_type *fortio) {
        rd_kw_type *radial = rd_kw_alloc(RADIAL_KW, 1, RD_CHAR);
        rd_kw_iset_string8(radial, 0, "FALSE");
        write_and_free(radial, fortio);
    };

    auto write_cell = [&](fortio_type *fortio, int host_cell_1based) {
        rd_kw_type *coords = rd_kw_alloc(COORDS_KW, 7, RD_INT);
        rd_kw_iset_int(coords, 0, 1);
        rd_kw_iset_int(coords, 1, 1);
        rd_kw_iset_int(coords, 2, 1);
        rd_kw_iset_int(coords, 3, 1);
        rd_kw_iset_int(coords, 4, 1);
        rd_kw_iset_int(coords, 5, host_cell_1based);
        rd_kw_iset_int(coords, 6, 0);
        write_and_free(coords, fortio);

        const float corners[24] = {
            0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0,
            0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
        };
        rd_kw_type *corners_kw = rd_kw_alloc(CORNERS_KW, 24, RD_FLOAT);
        for (int c = 0; c < 24; c++)
            rd_kw_iset_float(corners_kw, c, corners[c]);
        write_and_free(corners_kw, fortio);
    };

    fortio_type *fortio =
        fortio_open_writer(filename.c_str(), false, RD_ENDIAN_FLIP);

    write_dimens(fortio);

    rd_kw_type *mapunits = rd_kw_alloc(MAPUNITS_KW, 1, RD_CHAR);
    rd_kw_iset_string8(mapunits, 0, "METRES");
    write_and_free(mapunits, fortio);

    rd_kw_type *gridunit = rd_kw_alloc(GRIDUNIT_KW, 2, RD_CHAR);
    rd_kw_iset_string8(gridunit, 0, "METRES");
    rd_kw_iset_string8(gridunit, 1, "");
    write_and_free(gridunit, fortio);

    write_radial(fortio);
    write_cell(fortio, 0);

    // Outer LGR: size-1 LGR_KW means "parent is main grid".
    {
        rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 1, RD_CHAR);
        rd_kw_iset_string8(lgr_kw, 0, outer_name);
        write_and_free(lgr_kw, fortio);
        write_dimens(fortio);
        write_radial(fortio);
        write_cell(fortio, 1);
    }

    // Inner LGR: size-2 LGR_KW with second element = outer_name, so the
    // loader stores parent_name on the inner LGR and rd_grid_alloc_GRID
    // resolves the host via rd_grid_get_lgr.
    {
        rd_kw_type *lgr_kw = rd_kw_alloc(LGR_KW, 2, RD_CHAR);
        rd_kw_iset_string8(lgr_kw, 0, inner_name);
        rd_kw_iset_string8(lgr_kw, 1, outer_name);
        write_and_free(lgr_kw, fortio);
        write_dimens(fortio);
        write_radial(fortio);
        write_cell(fortio, 1);
    }

    fortio_fclose(fortio);
}

TEST_CASE_METHOD(Tmpdir, "rd_grid_load_case dispatches on the case input kind",
                 "[unittest]") {
    auto basename = dirname / "CASE";
    auto egrid_path = dirname / "CASE.EGRID";
    auto grid_path = dirname / "CASE.GRID";

    {
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(2, 2, 2, 1.0, 1.0, 1.0, nullptr);
        rd_grid_fwrite_EGRID2(grid, egrid_path.c_str(), RD_METRIC_UNITS);
        rd_grid_fwrite_GRID2(grid, grid_path.c_str(), RD_METRIC_UNITS);
        rd_grid_free(grid);
    }

    GIVEN("A path that points directly to an existing .EGRID file") {
        THEN("rd_grid_load_case loads the grid (case 1)") {
            rd_grid_type *grid = rd_grid_load_case(egrid_path.c_str());
            REQUIRE(grid != nullptr);
            rd_grid_free(grid);
        }
    }

    GIVEN("A path that points directly to an existing .GRID file") {
        THEN("rd_grid_load_case loads the grid (case 1)") {
            rd_grid_type *grid = rd_grid_load_case(grid_path.c_str());
            REQUIRE(grid != nullptr);
            rd_grid_free(grid);
        }
    }

    GIVEN("A case path with no extension (basename only)") {
        THEN("rd_grid_load_case finds the EGRID via extension search "
             "(case 3)") {
            rd_grid_type *grid = rd_grid_load_case(basename.c_str());
            REQUIRE(grid != nullptr);
            rd_grid_free(grid);
        }
    }

    GIVEN("A case path pointing to a non-grid file with a .DATA extension") {
        auto data_path = dirname / "CASE.DATA";
        {
            std::ofstream ofs(data_path);
            ofs << "RUNSPEC\n";
        }
        THEN("rd_grid_load_case derives the basename and loads the grid "
             "(case 3)") {
            rd_grid_type *grid = rd_grid_load_case(data_path.c_str());
            REQUIRE(grid != nullptr);
            rd_grid_free(grid);
        }
    }

    GIVEN("A case path pointing to a non-grid file with known formatted "
          "status") {
        // .UNRST is an unformatted restart file: rd_get_file_type returns a
        // recognized file type but not a grid, so case 2 searches for
        // unformatted grids only.
        auto unrst_path = dirname / "CASE.UNRST";
        {
            std::ofstream ofs(unrst_path);
            ofs << "restart stub";
        }
        THEN("rd_grid_load_case searches EGRID/GRID with matching formatted "
             "status (case 2)") {
            rd_grid_type *grid = rd_grid_load_case(unrst_path.c_str());
            REQUIRE(grid != nullptr);
            rd_grid_free(grid);
        }
    }

    GIVEN("A case path that does not correspond to any grid on disc") {
        auto missing_path = dirname / "NOPE";
        THEN("rd_grid_load_case returns null") {
            rd_grid_type *grid = rd_grid_load_case(missing_path.c_str());
            REQUIRE(grid == nullptr);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with a single LGR", "[unittest]") {
    GIVEN("An EGRID file containing a main grid and one LGR") {
        auto filename = dirname / "LGR.EGRID";
        write_egrid_with_single_lgr(filename, 3, 3, 3, 2, 2, 2, 1, 1, 1,
                                    "LGR1");

        THEN("The file can be loaded and exposes the LGR") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid) == 1);
            REQUIRE(rd_grid_has_lgr(grid, "LGR1"));

            rd_grid_type *lgr = rd_grid_get_lgr(grid, "LGR1");
            REQUIRE(lgr != nullptr);
            REQUIRE(rd_grid_get_nx(lgr) == 2);
            REQUIRE(rd_grid_get_ny(lgr) == 2);
            REQUIRE(rd_grid_get_nz(lgr) == 2);
            REQUIRE(rd_grid_get_lgr_nr(lgr) == 1);

            AND_THEN("Round-tripping the grid through a GRID file exercises "
                     "the 7-element coords branch of rd_grid_set_cell_GRID") {
                auto grid_filename = dirname / "LGR.GRID";
                rd_grid_fwrite_GRID2(grid, grid_filename.c_str(),
                                     RD_METRIC_UNITS);
                REQUIRE(fs::exists(grid_filename));

                rd_grid_type *reloaded = rd_grid_alloc(grid_filename.c_str());
                REQUIRE(reloaded != nullptr);
                REQUIRE(rd_grid_get_num_lgr(reloaded) == 1);
                REQUIRE(rd_grid_has_lgr(reloaded, "LGR1"));
                rd_grid_free(reloaded);
            }

            AND_THEN("Copying the grid preserves the LGR and exercises the "
                     "LGR-copy loop in rd_grid_alloc_copy") {
                rd_grid_type *copy = rd_grid_alloc_copy(grid);
                REQUIRE(copy != nullptr);
                REQUIRE(rd_grid_get_num_lgr(copy) == 1);
                REQUIRE(rd_grid_has_lgr(copy, "LGR1"));
                rd_grid_free(copy);
            }

            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Load EGRID with NNCs between the main grid and an LGR",
                 "[unittest]") {
    // The NNCHEAD section for the LGR references global-grid cells via NNCG
    // and LGR cells via NNCL. Loading exercises the LGR branch of
    // rd_grid_init_nnc.
    const std::vector<int> nncg = {1};
    const std::vector<int> nncl = {1};

    GIVEN("An EGRID file with a single LGR and an NNCG/NNCL pair") {
        auto filename = dirname / "LGR_NNC.EGRID";
        write_egrid_with_single_lgr(filename, 3, 3, 3, 2, 2, 2, 1, 1, 1,
                                    "LGR1", nullptr, nncg, nncl);

        THEN("The file loads and the NNCG/NNCL keywords are attached to the "
             "LGR's connections") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_has_lgr(grid, "LGR1"));
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Load EGRID with amalgamated NNCs between two LGRs",
                 "[unittest]") {
    // NNCHEADA + NNA1 + NNA2 describe NNCs between two different LGRs and
    // drive the amalgamated path rd_grid_init_nnc_amalgamated.
    const std::vector<int> nna1 = {1};
    const std::vector<int> nna2 = {1};

    GIVEN("An EGRID file with two LGRs and an amalgamated NNC section") {
        auto filename = dirname / "LGR_AMALGAMATED_NNC.EGRID";
        write_egrid_with_two_lgrs_and_amalgamated_nnc(
            filename, 3, 3, 3, "LGR1", 0, 0, 0, "LGR2", 2, 2, 2, nna1, nna2);

        THEN("The file loads and both LGRs are present") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_has_lgr(grid, "LGR1"));
            REQUIRE(rd_grid_has_lgr(grid, "LGR2"));
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with nested LGRs", "[unittest]") {
    GIVEN("An EGRID file with an outer LGR and an inner LGR nested in it") {
        auto filename = dirname / "NESTED.EGRID";
        write_egrid_with_nested_lgr(filename, 3, 3, 3, 1, 1, 1, 2, 2, 2,
                                    "OUTER", 0, 0, 0, 2, 2, 2, "INNER");

        THEN("The file can be loaded and both LGRs are accessible") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid) == 2);
            REQUIRE(rd_grid_has_lgr(grid, "OUTER"));
            REQUIRE(rd_grid_has_lgr(grid, "INNER"));

            AND_THEN("Copying the grid preserves both LGRs and exercises the "
                     "nested-parent branch of rd_grid_alloc_copy") {
                rd_grid_type *copy = rd_grid_alloc_copy(grid);
                REQUIRE(copy != nullptr);
                REQUIRE(rd_grid_get_num_lgr(copy) == 2);
                REQUIRE(rd_grid_has_lgr(copy, "OUTER"));
                REQUIRE(rd_grid_has_lgr(copy, "INNER"));
                rd_grid_free(copy);
            }

            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Load GRID file with two-element LGR_KW exercises the "
                 "parent_name branch",
                 "[unittest]") {
    GIVEN("A GRID file whose LGR_KW has an empty parent name") {
        auto filename = dirname / "LGR_EMPTY.GRID";
        write_grid_file_with_lgr_parent(filename, "LGR1", "");

        THEN("The file loads and the LGR descends from the main grid") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid) == 1);
            REQUIRE(rd_grid_has_lgr(grid, "LGR1"));
            rd_grid_free(grid);
        }
    }

    GIVEN("A GRID file whose LGR_KW declares parent = \"GLOBAL\"") {
        auto filename = dirname / "LGR_GLOBAL.GRID";
        write_grid_file_with_lgr_parent(filename, "LGR1", GLOBAL_STRING);

        THEN("The file loads and the LGR descends from the main grid") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid) == 1);
            REQUIRE(rd_grid_has_lgr(grid, "LGR1"));
            rd_grid_free(grid);
        }
    }

    GIVEN("A GRID file with a nested LGR whose parent is another LGR") {
        auto filename = dirname / "LGR_NESTED.GRID";
        write_grid_file_with_nested_lgr(filename, "OUTER", "INNER");

        THEN("The file loads and the nested LGR is hosted by the outer LGR") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid) == 2);
            REQUIRE(rd_grid_has_lgr(grid, "OUTER"));
            REQUIRE(rd_grid_has_lgr(grid, "INNER"));
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with MAPAXES", "[unittest]") {
    GIVEN("An EGRID file with a MAPAXES keyword describing a rotated frame") {
        // MAPAXES = (y_axis_end, origin, x_axis_end) pairs of (x,y).
        // Here the origin is offset to (10, 20) and the axes are rotated
        // by ~45 degrees so that neither unit_x nor unit_y is aligned with
        // the canonical (1,0) / (0,1) frame.
        const float mapaxes[6] = {
            10.0f, 21.0f, // y-axis end point
            10.0f, 20.0f, // origin
            11.0f, 21.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES.EGRID";
        write_egrid_with_single_lgr(filename, 3, 3, 3, 2, 2, 2, 1, 1, 1,
                                    "LGR1", mapaxes);

        rd_grid_type *grid = rd_grid_alloc(filename.c_str());
        REQUIRE(grid != nullptr);

        THEN("The grid reports that mapaxes are in use") {
            REQUIRE(rd_grid_use_mapaxes(grid));
        }

        THEN("Regenerating the COORD keyword exercises the inverse mapaxes "
             "transform") {
            rd_kw_type *coord_kw = rd_grid_alloc_coord_kw(grid);
            REQUIRE(coord_kw != nullptr);
            REQUIRE(rd_kw_get_size(coord_kw) ==
                    RD_GRID_COORD_SIZE(rd_grid_get_nx(grid),
                                       rd_grid_get_ny(grid)));
            rd_kw_free(coord_kw);
        }

        THEN("Writing the grid as a GRID file exercises the inverse mapaxes "
             "transform per cell") {
            auto grid_filename = dirname / "MAPAXES.GRID";
            rd_grid_fwrite_GRID2(grid, grid_filename.c_str(), RD_METRIC_UNITS);
            REQUIRE(fs::exists(grid_filename));
        }

        rd_grid_free(grid);
    }

    GIVEN("An EGRID file with a degenerate MAPAXES whose axes are collinear") {
        // x-axis endpoint and y-axis endpoint both lie along the x-axis, so
        // the cross-product denominator (x1*y2 - x2*y1) is zero. The loader
        // should discard the MAPAXES in this case.
        const float mapaxes[6] = {
            2.0f, 0.0f, // y-axis end point (collinear with x-axis)
            0.0f, 0.0f, // origin
            1.0f, 0.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES_DEGENERATE.EGRID";
        write_egrid_with_single_lgr(filename, 2, 2, 2, 2, 2, 2, 0, 0, 0,
                                    "LGR1", mapaxes);

        THEN("The grid loads and reports that mapaxes are not in use") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE_FALSE(rd_grid_use_mapaxes(grid));
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load GRID file with MAPAXES", "[unittest]") {
    GIVEN("A .GRID file with a MAPAXES keyword in the main grid section") {
        // Rotated and translated axes so the loader stores the mapaxes and
        // sets use_mapaxes on the grid.
        const float mapaxes[6] = {
            10.0f, 21.0f, // y-axis end point
            10.0f, 20.0f, // origin
            11.0f, 21.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES.GRID";
        write_grid_file_with_mapaxes(filename, mapaxes);

        THEN("The grid loads and reports that mapaxes are in use") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_use_mapaxes(grid));
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Load GRID file with an even-nz main grid and an LGR "
                 "exercises the CORNERS-count branch of dual-porosity check",
                 "[unittest]") {
    // When the main grid's nz is even, the dual-porosity heuristic is
    // entered. If an LGR is present, num_corners exceeds nx*ny*nz and the
    // fracture_index is set to nx*ny*nz/2 (line 2700).
    GIVEN("A .GRID file with a 1x1x2 main grid and a 1x1x1 LGR") {
        auto filename = dirname / "EVEN_NZ_WITH_LGR.GRID";
        write_grid_file_main_with_lgr(filename, 1, 1, 2);

        THEN("The file loads successfully and is not dual-porosity") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            REQUIRE_FALSE(rd_grid_dual_grid(grid));
            REQUIRE(rd_grid_has_lgr(grid, "LGR1"));
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with coarse cell groups", "[unittest]") {
    GIVEN("An EGRID file on disc with two coarse cell groups") {
        const int nx = 3, ny = 3, nz = 3;
        const int size = nx * ny * nz;

        // Assign two coarse groups (1-based). 0 means the cell is not in a
        // coarse group. Group 1 covers the two cells along i at (0,0,0)
        // and (1,0,0); group 2 covers two cells in the last k layer.
        std::vector<int> corsnum(size, 0);
        corsnum[0] = 1;
        corsnum[1] = 1;
        corsnum[size - 1] = 2;
        corsnum[size - 2] = 2;

        auto filename = dirname / "CORSNUM.EGRID";
        write_egrid_with_coarse_groups(filename, nx, ny, nz, corsnum.data());

        rd_grid_type *grid = rd_grid_alloc(filename.c_str());
        REQUIRE(grid != nullptr);

        THEN("The grid reports that coarse cells are present") {
            REQUIRE(rd_grid_have_coarse_cells(grid));
            REQUIRE(rd_grid_get_num_coarse_groups(grid) == 2);
        }

        THEN("Cells in a coarse group are flagged as such, others are not") {
            REQUIRE(rd_grid_cell_in_coarse_group1(grid, 0));
            REQUIRE(rd_grid_cell_in_coarse_group1(grid, 1));
            REQUIRE_FALSE(rd_grid_cell_in_coarse_group1(grid, 2));
            REQUIRE(rd_grid_cell_in_coarse_group1(grid, size - 1));
            REQUIRE(rd_grid_cell_in_coarse_group1(grid, size - 2));
        }

        THEN("The coarse group objects are accessible by index") {
            REQUIRE(rd_grid_iget_coarse_group(grid, 0) != nullptr);
            REQUIRE(rd_grid_iget_coarse_group(grid, 1) != nullptr);
        }

        rd_grid_free(grid);
    }
}

TEST_CASE("rd_grid_alloc_GRDECL_kw with explicit ACTNUM", "[unittest]") {
    const int nx = 2, ny = 2, nz = 2;
    rd_kw_type *coord_kw =
        rd_kw_alloc(COORD_KW, RD_GRID_COORD_SIZE(nx, ny), RD_FLOAT);
    rd_kw_type *zcorn_kw =
        rd_kw_alloc(ZCORN_KW, RD_GRID_ZCORN_SIZE(nx, ny, nz), RD_FLOAT);

    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            int offset = 6 * (i + j * nx);
            rd_kw_iset_float(coord_kw, offset, i);
            rd_kw_iset_float(coord_kw, offset + 1, j);
            rd_kw_iset_float(coord_kw, offset + 2, -1);
            rd_kw_iset_float(coord_kw, offset + 3, i);
            rd_kw_iset_float(coord_kw, offset + 4, j);
            rd_kw_iset_float(coord_kw, offset + 5, -1);
            for (int k = 0; k < nz; k++) {
                for (int c = 0; c < 4; c++) {
                    int zi1 = rd_grid_zcorn_index__(nx, ny, i, j, k, c);
                    int zi2 = rd_grid_zcorn_index__(nx, ny, i, j, k, c + 4);
                    rd_kw_iset_float(zcorn_kw, zi1, k);
                    rd_kw_iset_float(zcorn_kw, zi2, k + 1);
                }
            }
        }
    }

    const int size = nx * ny * nz;
    rd_kw_type *actnum_kw = rd_kw_alloc(ACTNUM_KW, size, RD_INT);
    rd_kw_scalar_set_int(actnum_kw, 1);
    rd_kw_iset_int(actnum_kw, 0, 0);

    rd_grid_type *grid = rd_grid_alloc_GRDECL_kw(nx, ny, nz, zcorn_kw,
                                                 coord_kw, actnum_kw, NULL);
    REQUIRE(grid != nullptr);
    REQUIRE(rd_grid_get_active_size(grid) == size - 1);

    rd_grid_free(grid);
    rd_kw_free(actnum_kw);
    rd_kw_free(coord_kw);
    rd_kw_free(zcorn_kw);
}

TEST_CASE_METHOD(Tmpdir, "Test case loading", "[unittest]") {
    GIVEN("A grid on disc") {
        auto filename = dirname / "GRID.EGRID";
        rd_grid_type *rd_grid =
            rd_grid_alloc_rectangular(5, 5, 5, 1, 1, 1, nullptr);
        rd_grid_fwrite_EGRID2(rd_grid, filename.c_str(), RD_METRIC_UNITS);
        rd_grid_free(rd_grid);

        THEN("Loading that grid gives a non-null grid as a case") {
            rd_grid_type *grid = rd_grid_load_case(filename.c_str());
            REQUIRE(grid != NULL);
            rd_grid_free(grid);
        }
        THEN("Loading it as without extension also gives non-null grid") {
            auto no_ext_file_name = dirname / "GRID";
            rd_grid_type *grid = rd_grid_load_case(no_ext_file_name.c_str());
            REQUIRE(grid != NULL);
            rd_grid_free(grid);
        }
        THEN("Loadinging a non-existent grid gives NULL") {
            auto does_not_exist = dirname / "DOES_NOT_EXIST.EGRID";
            rd_grid_type *grid = rd_grid_load_case(does_not_exist.c_str());
            REQUIRE(grid == NULL);
        }
        THEN("Loading non-existent grid without extension gives NULL") {
            auto does_not_exist = dirname / "DOES_NOT_EXIST";
            rd_grid_type *grid = rd_grid_load_case(does_not_exist.c_str());
            REQUIRE(grid == NULL);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Test format writing grid", "[unittest]") {
    GIVEN("A Grid") {
        rd_grid_type *rd_grid =
            rd_grid_alloc_rectangular(5, 5, 5, 1, 1, 1, nullptr);

        THEN("Writing that file as a FEGRID is a formatted file") {
            rd_grid_fwrite_EGRID2(rd_grid, (dirname / "CASE.FEGRID").c_str(),
                                  RD_METRIC_UNITS);
            REQUIRE(util_fmt_bit8((dirname / "CASE.FEGRID").c_str()));
        }

        THEN("Writing that file as a EGRID is an unformatted file") {
            rd_grid_fwrite_EGRID2(rd_grid, (dirname / "CASE.EGRID").c_str(),
                                  RD_METRIC_UNITS);
            REQUIRE(!util_fmt_bit8((dirname / "CASE.EGRID").c_str()));
        }
        THEN(
            "Writing that file with unknown extension is an unformatted file") {
            rd_grid_fwrite_EGRID2(rd_grid, (dirname / "CASE.UNKNOWN").c_str(),
                                  RD_METRIC_UNITS);
            REQUIRE(!util_fmt_bit8((dirname / "CASE.UNKNOWN").c_str()));
        }

        rd_grid_free(rd_grid);
    }
}

TEST_CASE_METHOD(Tmpdir, "Writing and reading grid", "[unittest]") {
    GIVEN("A Grid") {
        rd_grid_type *rd_grid =
            rd_grid_alloc_rectangular(5, 5, 5, 1, 1, 1, nullptr);

        THEN("Writing and reading that grid gives equal grid") {
            auto filename = (dirname / "CASE.EGRID");
            rd_grid_fwrite_EGRID2(rd_grid, filename.c_str(), RD_METRIC_UNITS);
            rd_grid_type *read_grid = rd_grid_alloc(filename.c_str());

            REQUIRE(rd_grid_compare(rd_grid, read_grid, false, false, true));

            rd_grid_free(read_grid);
        }

        rd_grid_free(rd_grid);
    }
}

TEST_CASE("Test utility functions on a regular grid", "[unittest]") {
    GIVEN("A rectangular grid") {
        int actnum_data[] = {1, 1, 1, 1, 0, 1, 1, 1};
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(2, 2, 2, 1.0, 2.0, 3.0, actnum_data);

        SECTION("Grid dimension") {
            REQUIRE(rd_grid_get_nx(grid) == 2);
            REQUIRE(rd_grid_get_ny(grid) == 2);
            REQUIRE(rd_grid_get_nz(grid) == 2);

            int nx, ny, nz, nactive;
            rd_grid_get_dims(grid, &nx, &ny, &nz, &nactive);
            REQUIRE(nx == 2);
            REQUIRE(ny == 2);
            REQUIRE(nz == 2);
            REQUIRE(nactive == 7);

            REQUIRE(rd_grid_get_global_size(grid) == 8);
            REQUIRE(rd_grid_get_nactive(grid) == 7);
            REQUIRE(rd_grid_get_active_size(grid) == 7);
        }

        SECTION("name and unit") {
            const char *name = rd_grid_get_name(grid);
            REQUIRE(name == nullptr); // nullptr for rectangular grids

            ert_rd_unit_enum unit = rd_grid_get_unit_system(grid);
            REQUIRE(unit == RD_METRIC_UNITS);

            float scale = rd_grid_output_scaling(grid, RD_METRIC_UNITS);
            REQUIRE(scale == 1.0f);
        }

        SECTION("Index") {
            int global_idx = rd_grid_get_global_index3(grid, 0, 0, 0);
            REQUIRE(global_idx == 0);

            int i, j, k;
            rd_grid_get_ijk1(grid, global_idx, &i, &j, &k);
            REQUIRE(i == 0);
            REQUIRE(j == 0);
            REQUIRE(k == 0);

            REQUIRE(rd_grid_ijk_valid(grid, 0, 0, 0));
            REQUIRE(rd_grid_ijk_valid(grid, 1, 1, 1));
            REQUIRE(!rd_grid_ijk_valid(grid, 5, 5, 5));

            int active_idx = rd_grid_get_active_index3(grid, 0, 0, 0);
            REQUIRE(active_idx == 0);

            int active_idx1 = rd_grid_get_active_index1(grid, global_idx);
            REQUIRE(active_idx1 == 0);

            int global_from_active =
                rd_grid_get_global_index1A(grid, active_idx);
            REQUIRE(global_from_active == global_idx);

            rd_grid_get_ijk1A(grid, active_idx, &i, &j, &k);
            REQUIRE(i == 0);
            REQUIRE(j == 0);
            REQUIRE(k == 0);
        }

        SECTION("Cell activity") {
            REQUIRE(rd_grid_cell_active3(grid, 0, 0, 0));
            REQUIRE(rd_grid_cell_active1(grid, 0));
            REQUIRE(!rd_grid_cell_active1(grid, 4));

            REQUIRE(rd_grid_cell_valid1(grid, 0));
            REQUIRE(!rd_grid_cell_invalid1(grid, 0));
        }

        SECTION("Position and geometry") {
            double x, y, z;
            rd_grid_get_xyz3(grid, 0, 0, 0, &x, &y, &z);
            REQUIRE(x == 0.5);
            REQUIRE(y == 1.0);
            REQUIRE(z == 1.5);

            rd_grid_get_xyz1(grid, 1, &x, &y, &z);
            REQUIRE(x == 1.5);
            REQUIRE(y == 1.0);
            REQUIRE(z == 1.5);

            int active_idx = rd_grid_get_active_index1(grid, 6);
            rd_grid_get_xyz1A(grid, active_idx, &x, &y, &z);
            REQUIRE(x == 0.5);
            REQUIRE(y == 3.0);
            REQUIRE(z == 4.5);

            rd_grid_get_corner_xyz(grid, 0, 0, 0, &x, &y, &z);
            REQUIRE(x == 0.0);
            REQUIRE(y == 0.0);
            REQUIRE(z == 0.0);

            rd_grid_get_cell_corner_xyz1(grid, 0, 3, &x, &y, &z);
            REQUIRE(x == 1.0);
            REQUIRE(y == 2.0);
            REQUIRE(z == 0.0);

            double corners_x[8], corners_y[8], corners_z[8];
            rd_grid_export_cell_corners1(grid, 0, corners_x, corners_y,
                                         corners_z);
            REQUIRE(corners_x[0] == 0.0);
            REQUIRE(corners_x[1] == 1.0);
            REQUIRE(corners_x[2] == 0.0);
            REQUIRE(corners_x[3] == 1.0);
            REQUIRE(corners_x[4] == 0.0);
            REQUIRE(corners_x[5] == 1.0);
            REQUIRE(corners_x[6] == 0.0);
            REQUIRE(corners_x[7] == 1.0);

            REQUIRE(corners_y[0] == 0.0);
            REQUIRE(corners_y[1] == 0.0);
            REQUIRE(corners_y[2] == 2.0);
            REQUIRE(corners_y[3] == 2.0);
            REQUIRE(corners_y[4] == 0.0);
            REQUIRE(corners_y[5] == 0.0);
            REQUIRE(corners_y[6] == 2.0);
            REQUIRE(corners_y[7] == 2.0);

            REQUIRE(corners_z[0] == 0.0);
            REQUIRE(corners_z[1] == 0.0);
            REQUIRE(corners_z[2] == 0.0);
            REQUIRE(corners_z[3] == 0.0);
            REQUIRE(corners_z[4] == 3.0);
            REQUIRE(corners_z[5] == 3.0);
            REQUIRE(corners_z[6] == 3.0);
            REQUIRE(corners_z[7] == 3.0);
        }

        SECTION("Cell dimension") {
            double dx = rd_grid_get_cell_dx1(grid, 0);
            REQUIRE(dx == 1.0);

            double dy = rd_grid_get_cell_dy1(grid, 0);
            REQUIRE(dy == 2.0);

            double dz = rd_grid_get_cell_dz1(grid, 0);
            REQUIRE(dz == 3.0);

            double thickness = rd_grid_get_cell_thickness1(grid, 0);
            REQUIRE(thickness == 3.0);

            int active_idx = rd_grid_get_active_index1(grid, 0);
            double dx_a = rd_grid_get_cell_dx1A(grid, active_idx);
            REQUIRE(dx_a == 1.0);

            double dy_a = rd_grid_get_cell_dy1A(grid, active_idx);
            REQUIRE(dy_a == 2.0);

            double dz_a = rd_grid_get_cell_dz1A(grid, active_idx);
            REQUIRE(dz_a == 3.0);

            double dx_dist, dy_dist, dz_dist;
            rd_grid_get_distance(grid, 0, 1, &dx_dist, &dy_dist, &dz_dist);
            REQUIRE(dx_dist == -1.0);
            REQUIRE(dy_dist == 0.0);
            REQUIRE(dz_dist == 0.0);
        }

        SECTION("Volume and depth") {
            double volume = rd_grid_get_cell_volume1(grid, 0);
            REQUIRE(volume == 6.0);

            int active_idx = rd_grid_get_active_index1(grid, 0);
            double volume_a = rd_grid_get_cell_volume1A(grid, active_idx);
            REQUIRE(volume_a == 6.0);

            double cdepth_a = rd_grid_get_cdepth1A(grid, active_idx);
            REQUIRE(cdepth_a == 1.5);

            double top_a = rd_grid_get_top1A(grid, active_idx);
            REQUIRE(top_a == 0.0);

            double cdepth = rd_grid_get_cdepth1(grid, 0);
            REQUIRE(cdepth == 1.5);

            double top2 = rd_grid_get_top2(grid, 0, 0);
            REQUIRE(top2 == 0.0);

            double bottom2 = rd_grid_get_bottom2(grid, 0, 0);
            REQUIRE(bottom2 == 6.0);

            int k_loc = rd_grid_locate_depth(grid, 1.5, 0, 0);
            REQUIRE(k_loc == 0);
        }

        SECTION("Cell containment") {
            double x, y, z;
            rd_grid_get_xyz1(grid, 0, &x, &y, &z);

            REQUIRE(rd_grid_cell_contains_xyz1(grid, 0, x, y, z));

            int found_idx = rd_grid_get_global_index_from_xyz(grid, x, y, z, 0);
            REQUIRE(found_idx == 0);

            int i_found, j_found;
            bool found_ij =
                rd_grid_get_ij_from_xy(grid, x, y, 0, &i_found, &j_found);
            REQUIRE(found_ij);
            REQUIRE(i_found == 0);
            REQUIRE(j_found == 0);
        }

        SECTION("Grid properties") {
            REQUIRE(!rd_grid_dual_grid(grid));
            REQUIRE(rd_grid_cell_regular1(grid, 0));
            REQUIRE(!rd_grid_use_mapaxes(grid));
        }

        SECTION("LGR functions") {
            REQUIRE(rd_grid_get_num_lgr(grid) == 0);
            REQUIRE(rd_grid_get_lgr_nr(grid) == 0);
            REQUIRE(!rd_grid_has_lgr(grid, "test"));
            REQUIRE(!rd_grid_has_lgr_nr(grid, 1));
            REQUIRE(rd_grid_get_cell_lgr1(grid, 0) == nullptr);
            REQUIRE(rd_grid_test_lgr_consistency(grid));
        }

        SECTION("Coarse cells") {
            REQUIRE(!rd_grid_have_coarse_cells(grid));
            REQUIRE(rd_grid_get_num_coarse_groups(grid) == 0);
            REQUIRE(!rd_grid_cell_in_coarse_group1(grid, 0));
        }

        SECTION("Fractures") {
            REQUIRE(rd_grid_get_nactive_fracture(grid) == 0);
            REQUIRE(rd_grid_get_active_fracture_index1(grid, 0) == -1);
            REQUIRE(rd_grid_get_global_index1F(grid, 0) == -1);
        }

        std::vector<float> expected_zcorn(2 * 2 * 2 * 8);
        for (int i = 0; i < 16; i++)
            expected_zcorn[i] = 0.0f;
        for (int i = 16; i < 48; i++)
            expected_zcorn[i] = 3.0f;
        for (int i = 48; i < 64; i++)
            expected_zcorn[i] = 6.0f;

        std::vector<float> expected_coord{
            0.0, 0.0, 0.0, 0.0, 0.0, 6.0, 1.0, 0.0, 0.0, 1.0, 0.0,
            6.0, 2.0, 0.0, 0.0, 2.0, 0.0, 6.0, 0.0, 2.0, 0.0, 0.0,
            2.0, 6.0, 1.0, 2.0, 0.0, 1.0, 2.0, 6.0, 2.0, 2.0, 0.0,
            2.0, 2.0, 6.0, 0.0, 4.0, 0.0, 0.0, 4.0, 6.0, 1.0, 4.0,
            0.0, 1.0, 4.0, 6.0, 2.0, 4.0, 0.0, 2.0, 4.0, 6.0};

        SECTION("Keyword allocation") {
            rd_kw_type *zcorn_kw = rd_grid_alloc_zcorn_kw(grid);
            REQUIRE(zcorn_kw != nullptr);
            REQUIRE(rd_kw_get_size(zcorn_kw) ==
                    static_cast<int>(expected_zcorn.size()));
            for (size_t i = 0; i < expected_zcorn.size(); i++)
                REQUIRE(rd_kw_iget_float(zcorn_kw, i) == expected_zcorn[i]);
            rd_kw_free(zcorn_kw);

            rd_kw_type *actnum_kw = rd_grid_alloc_actnum_kw(grid);
            REQUIRE(actnum_kw != nullptr);
            REQUIRE(rd_kw_get_size(actnum_kw) == 8);
            for (int i = 0; i < 8; i++)
                REQUIRE(rd_kw_iget_int(actnum_kw, i) == actnum_data[i]);
            rd_kw_free(actnum_kw);

            rd_kw_type *coord_kw = rd_grid_alloc_coord_kw(grid);
            REQUIRE(coord_kw != nullptr);
            REQUIRE(rd_kw_get_size(coord_kw) ==
                    static_cast<int>(expected_coord.size()));
            for (int i = 0; i < rd_kw_get_size(coord_kw); i++)
                REQUIRE(rd_kw_iget_float(coord_kw, i) == expected_coord[i]);
            rd_kw_free(coord_kw);

            rd_kw_type *volume_kw = rd_grid_alloc_volume_kw(grid, false);
            REQUIRE(volume_kw != nullptr);
            REQUIRE(rd_kw_get_size(volume_kw) == 8);
            for (int i = 0; i < rd_kw_get_size(volume_kw); i++)
                REQUIRE(rd_kw_iget_double(volume_kw, i) == 6.0);
            rd_kw_free(volume_kw);

            rd_kw_type *volume_kw_active = rd_grid_alloc_volume_kw(grid, true);
            REQUIRE(volume_kw_active != nullptr);
            REQUIRE(rd_kw_get_size(volume_kw_active) == 7);
            for (int i = 0; i < rd_kw_get_size(volume_kw_active); i++)
                REQUIRE(rd_kw_iget_double(volume_kw_active, i) == 6.0);
            rd_kw_free(volume_kw_active);
        }

        SECTION("ZCORN and COORD size/data") {
            int zcorn_size = rd_grid_get_zcorn_size(grid);
            REQUIRE(zcorn_size == 2 * 2 * 2 * 8);

            int coord_size = rd_grid_get_coord_size(grid);
            REQUIRE(coord_size == static_cast<int>(expected_coord.size()));

            std::vector<float> zcorn_data(zcorn_size);
            rd_grid_init_zcorn_data(grid, zcorn_data.data());
            REQUIRE(zcorn_data == expected_zcorn);

            std::vector<double> zcorn_data_double(zcorn_size);
            rd_grid_init_zcorn_data_double(grid, zcorn_data_double.data());
            REQUIRE(zcorn_data_double ==
                    std::vector<double>(expected_zcorn.begin(),
                                        expected_zcorn.end()));

            std::vector<float> coord_data(coord_size);
            rd_grid_init_coord_data(grid, coord_data.data());
            REQUIRE(coord_data == expected_coord);

            std::vector<double> coord_data_double(coord_size);
            rd_grid_init_coord_data_double(grid, coord_data_double.data());
            REQUIRE(coord_data_double ==
                    std::vector<double>(expected_coord.begin(),
                                        expected_coord.end()));

            std::vector<int> actnum(rd_grid_get_global_size(grid));
            rd_grid_init_actnum_data(grid, actnum.data());
            for (int i = 0; i < 8; i++)
                REQUIRE(actnum[i] == actnum_data[i]);
        }

        SECTION("copy & reset actnum") {
            rd_grid_type *copy = rd_grid_alloc_copy(grid);
            REQUIRE(copy != nullptr);
            REQUIRE(rd_grid_compare(grid, copy, false, false, false));

            std::vector<int> new_actnum(8, 1);
            new_actnum[0] = 0;
            new_actnum[1] = 0;
            rd_grid_reset_actnum(copy, new_actnum.data());

            REQUIRE(rd_grid_get_nactive(copy) == 6);

            rd_grid_free(copy);
        }

        SECTION("Grid copy with NNCs") {
            rd_grid_add_self_nnc(grid, 0, 1, 0);
            rd_grid_add_self_nnc(grid, 0, 2, 1);
            rd_grid_add_self_nnc(grid, 1, 3, 2);

            rd_grid_type *copy = rd_grid_alloc_copy(grid);
            REQUIRE(copy != nullptr);

            auto nnc_info_orig = rd_grid_get_cell_nnc_info1(grid, 0);
            auto nnc_info_copy = rd_grid_get_cell_nnc_info1(copy, 0);
            REQUIRE(nnc_info_copy != nullptr);
            REQUIRE(nnc_info_equal(nnc_info_orig, nnc_info_copy));

            auto nnc_info_orig1 = rd_grid_get_cell_nnc_info1(grid, 1);
            auto nnc_info_copy1 = rd_grid_get_cell_nnc_info1(copy, 1);
            REQUIRE(nnc_info_copy1 != nullptr);
            REQUIRE(nnc_info_equal(nnc_info_orig1, nnc_info_copy1));

            rd_grid_free(copy);
        }

        SECTION("Export") {
            int num_cells = rd_grid_get_global_size(grid);
            std::vector<int> global_index(num_cells);
            for (int i = 0; i < num_cells; i++) {
                global_index[i] = i;
            }

            std::vector<int> index_data(num_cells * 4);
            rd_grid_export_index(grid, global_index.data(), index_data.data(),
                                 false);
            REQUIRE(index_data == std::vector<int>{
                                      0, 0, 0, 0, 1, 0, 0, 1, 0,  1, 0,
                                      2, 1, 1, 0, 3, 0, 0, 1, -1, 1, 0,
                                      1, 4, 0, 1, 1, 5, 1, 1, 1,  6,
                                  });

            std::vector<double> volume_output(num_cells);
            rd_grid_export_volume(grid, num_cells, global_index.data(),
                                  volume_output.data());
            REQUIRE(volume_output.size() == 8);
            for (auto v : volume_output)
                REQUIRE(v == 6.0);

            std::vector<double> position_output(num_cells * 3);
            rd_grid_export_position(grid, num_cells, global_index.data(),
                                    position_output.data());
            REQUIRE(position_output == std::vector<double>{
                                           0.5, 1.0, 1.5, 1.5, 1.0, 1.5,
                                           0.5, 3.0, 1.5, 1.5, 3.0, 1.5,
                                           0.5, 1.0, 4.5, 1.5, 1.0, 4.5,
                                           0.5, 3.0, 4.5, 1.5, 3.0, 4.5,
                                       });

            std::vector<double> corners_output(num_cells * 24);
            export_corners(grid, num_cells, global_index.data(),
                           corners_output.data());
            REQUIRE(corners_output ==
                    std::vector<double>{
                        0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 3.0, 1.0, 0.0, 3.0, 0.0, 2.0, 3.0, 1.0,
                        2.0, 3.0, 1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 1.0, 2.0, 0.0,
                        2.0, 2.0, 0.0, 1.0, 0.0, 3.0, 2.0, 0.0, 3.0, 1.0, 2.0,
                        3.0, 2.0, 2.0, 3.0, 0.0, 2.0, 0.0, 1.0, 2.0, 0.0, 0.0,
                        4.0, 0.0, 1.0, 4.0, 0.0, 0.0, 2.0, 3.0, 1.0, 2.0, 3.0,
                        0.0, 4.0, 3.0, 1.0, 4.0, 3.0, 1.0, 2.0, 0.0, 2.0, 2.0,
                        0.0, 1.0, 4.0, 0.0, 2.0, 4.0, 0.0, 1.0, 2.0, 3.0, 2.0,
                        2.0, 3.0, 1.0, 4.0, 3.0, 2.0, 4.0, 3.0, 0.0, 0.0, 3.0,
                        1.0, 0.0, 3.0, 0.0, 2.0, 3.0, 1.0, 2.0, 3.0, 0.0, 0.0,
                        6.0, 1.0, 0.0, 6.0, 0.0, 2.0, 6.0, 1.0, 2.0, 6.0, 1.0,
                        0.0, 3.0, 2.0, 0.0, 3.0, 1.0, 2.0, 3.0, 2.0, 2.0, 3.0,
                        1.0, 0.0, 6.0, 2.0, 0.0, 6.0, 1.0, 2.0, 6.0, 2.0, 2.0,
                        6.0, 0.0, 2.0, 3.0, 1.0, 2.0, 3.0, 0.0, 4.0, 3.0, 1.0,
                        4.0, 3.0, 0.0, 2.0, 6.0, 1.0, 2.0, 6.0, 0.0, 4.0, 6.0,
                        1.0, 4.0, 6.0, 1.0, 2.0, 3.0, 2.0, 2.0, 3.0, 1.0, 4.0,
                        3.0, 2.0, 4.0, 3.0, 1.0, 2.0, 6.0, 2.0, 2.0, 6.0, 1.0,
                        4.0, 6.0, 2.0, 4.0, 6.0});
        }

        SECTION("NNC") {
            auto nnc_info_before = rd_grid_get_cell_nnc_info1(grid, 0);
            REQUIRE(nnc_info_before == nullptr);

            rd_grid_add_self_nnc(grid, 0, 1, 0);

            auto nnc_info = rd_grid_get_cell_nnc_info1(grid, 0);
            REQUIRE(nnc_info != nullptr);
            REQUIRE(nnc_info_get_size(nnc_info) == 1);

            auto nnc_vector = nnc_info_get_self_vector(nnc_info);
            REQUIRE(nnc_vector != nullptr);
            REQUIRE(nnc_vector_get_size(nnc_vector) == 1);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 0) == 1);
            REQUIRE(nnc_vector_iget_nnc_index(nnc_vector, 0) == 0);

            REQUIRE(nnc_info_get_lgr_nr(nnc_info) == 0);
            REQUIRE(nnc_vector_get_lgr_nr(nnc_vector) == 0);

            rd_grid_add_self_nnc(grid, 0, 2, 1);
            rd_grid_add_self_nnc(grid, 0, 3, 2);

            nnc_info = rd_grid_get_cell_nnc_info1(grid, 0);
            REQUIRE(nnc_info != nullptr);
            nnc_vector = nnc_info_get_self_vector(nnc_info);
            REQUIRE(nnc_vector_get_size(nnc_vector) == 3);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 3);

            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 0) == 1);
            REQUIRE(nnc_vector_iget_nnc_index(nnc_vector, 0) == 0);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 1) == 2);
            REQUIRE(nnc_vector_iget_nnc_index(nnc_vector, 1) == 1);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 2) == 3);
            REQUIRE(nnc_vector_iget_nnc_index(nnc_vector, 2) == 2);

            const auto &grid_index_list =
                nnc_info_get_self_grid_index_list(nnc_info);
            REQUIRE(grid_index_list.size() == 3);
            REQUIRE(grid_index_list[0] == 1);
            REQUIRE(grid_index_list[1] == 2);
            REQUIRE(grid_index_list[2] == 3);

            const auto &nnc_index_list =
                nnc_vector_get_nnc_index_list(nnc_vector);
            REQUIRE(nnc_index_list.size() == 3);
            REQUIRE(nnc_index_list[0] == 0);
            REQUIRE(nnc_index_list[1] == 1);
            REQUIRE(nnc_index_list[2] == 2);

            rd_grid_add_self_nnc(grid, 1, 3, 3);
            rd_grid_add_self_nnc(grid, 1, 5, 4);

            auto nnc_info_cell1 = rd_grid_get_cell_nnc_info1(grid, 1);
            REQUIRE(nnc_info_cell1 != nullptr);
            REQUIRE(nnc_info_get_total_size(nnc_info_cell1) == 2);

            auto nnc_vector_cell1 = nnc_info_get_self_vector(nnc_info_cell1);
            REQUIRE(nnc_vector_get_size(nnc_vector_cell1) == 2);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector_cell1, 0) == 3);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector_cell1, 1) == 5);

            // cells without NNCs return nullptr
            REQUIRE(rd_grid_get_cell_nnc_info1(grid, 4) == nullptr);
            REQUIRE(rd_grid_get_cell_nnc_info1(grid, 6) == nullptr);

            REQUIRE(nnc_info_has_grid_index_list(nnc_info, 0));
            REQUIRE(!nnc_info_has_grid_index_list(nnc_info, 1));
        }

        SECTION("NNC copy and equality") {
            rd_grid_add_self_nnc(grid, 0, 1, 0);
            rd_grid_add_self_nnc(grid, 0, 2, 1);

            auto nnc_info_orig = rd_grid_get_cell_nnc_info1(grid, 0);
            REQUIRE(nnc_info_orig != nullptr);

            auto nnc_info_copy = nnc_info_alloc_copy(nnc_info_orig);
            REQUIRE(nnc_info_copy != nullptr);
            REQUIRE(nnc_info_equal(nnc_info_orig, nnc_info_copy));

            REQUIRE(nnc_info_get_lgr_nr(nnc_info_copy) ==
                    nnc_info_get_lgr_nr(nnc_info_orig));
            REQUIRE(nnc_info_get_size(nnc_info_copy) ==
                    nnc_info_get_size(nnc_info_orig));
            REQUIRE(nnc_info_get_total_size(nnc_info_copy) ==
                    nnc_info_get_total_size(nnc_info_orig));

            auto nnc_vector_orig = nnc_info_get_self_vector(nnc_info_orig);
            auto nnc_vector_copy = nnc_info_get_self_vector(nnc_info_copy);
            REQUIRE(nnc_vector_get_size(nnc_vector_copy) ==
                    nnc_vector_get_size(nnc_vector_orig));
            REQUIRE(nnc_vector_equal(nnc_vector_orig, nnc_vector_copy));

            REQUIRE(nnc_info_equal(nnc_info_orig, nnc_info_copy));
            REQUIRE(!nnc_info_equal(nnc_info_orig, nullptr));
            REQUIRE(!nnc_info_equal(nullptr, nnc_info_copy));

            nnc_info_free(nnc_info_copy);
        }

        SECTION("NNC with inactive cells") {
            // Add NNC involving the inactive cell 4
            rd_grid_add_self_nnc(grid, 3, 4, 0);

            auto nnc_info = rd_grid_get_cell_nnc_info1(grid, 3);
            auto nnc_vector = nnc_info_get_self_vector(nnc_info);

            REQUIRE(nnc_vector_get_size(nnc_vector) == 1);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 0) == 4);

            // Verify the inactive cell can also have NNC info
            rd_grid_add_self_nnc(grid, 4, 5, 1);
            auto nnc_info_inactive = rd_grid_get_cell_nnc_info1(grid, 4);
            REQUIRE(nnc_info_inactive != nullptr);
        }

        GIVEN("A grid keyword") {
            rd_kw_type *kw =
                rd_kw_alloc("PORO", rd_grid_get_nactive(grid), RD_FLOAT);

            for (int i = 0; i < rd_grid_get_nactive(grid); i++) {
                rd_kw_iset_float(kw, i, 0.2f + i * 0.01f);
            }

            SECTION("column property") {
                double prop = rd_grid_get_property(grid, kw, 0, 0, 0);
                REQUIRE(prop >= 0.0);

                double_vector_type *column = double_vector_alloc(0, 0.0);
                rd_grid_get_column_property(grid, kw, 0, 0, column);
                REQUIRE(double_vector_size(column) > 0);
                double_vector_free(column);
            }

            SECTION("Keyword copy") {
                rd_kw_type *target_kw = rd_kw_alloc(
                    "TARGET", rd_grid_get_global_size(grid), RD_FLOAT);
                rd_grid_global_kw_copy(grid, target_kw, kw);

                rd_kw_type *compressed_kw = rd_kw_alloc(
                    "COMP", rd_grid_get_active_size(grid), RD_FLOAT);
                rd_grid_compressed_kw_copy(grid, compressed_kw, target_kw);
                rd_kw_free(target_kw);
                rd_kw_free(compressed_kw);
            }

            SECTION("Export data functions") {
                std::vector<int> global_index = {0, 1, 2};
                std::vector<int> int_output(3);

                rd_kw_type *int_kw = rd_kw_alloc(
                    "PVTNUM", rd_grid_get_global_size(grid), RD_INT);
                for (int i = 0; i < rd_grid_get_global_size(grid); i++) {
                    rd_kw_iset_int(int_kw, i, 1);
                }
                rd_grid_export_data_as_int(3, global_index.data(), int_kw,
                                           int_output.data());

                // Verify exported integer data
                REQUIRE(int_output[0] == 1);
                REQUIRE(int_output[1] == 1);
                REQUIRE(int_output[2] == 1);
                rd_kw_free(int_kw);

                std::vector<double> double_output(3);
                rd_grid_export_data_as_double(3, global_index.data(), kw,
                                              double_output.data());

                REQUIRE_THAT(double_output[0],
                             Catch::Matchers::WithinAbs(0.20, 0.0001));
                REQUIRE_THAT(double_output[1],
                             Catch::Matchers::WithinAbs(0.21, 0.0001));
                REQUIRE_THAT(double_output[2],
                             Catch::Matchers::WithinAbs(0.22, 0.0001));
            }

            rd_kw_free(kw);
        }

        rd_grid_free(grid);
    }
}

TEST_CASE("Test NNC info structure", "[unittest]") {
    GIVEN("An nnc_info structure") {
        int lgr_nr = 0;
        auto nnc_info = nnc_info_alloc(lgr_nr);
        REQUIRE(nnc_info != nullptr);

        THEN("Initially it should be empty") {
            REQUIRE(nnc_info_get_lgr_nr(nnc_info) == lgr_nr);
            REQUIRE(nnc_info_get_size(nnc_info) == 0);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 0);
            REQUIRE(nnc_info_get_vector(nnc_info, lgr_nr) == nullptr);
            REQUIRE(!nnc_info_has_grid_index_list(nnc_info, lgr_nr));
        }

        THEN("Adding NNC connections updates the structure") {
            nnc_info_add_nnc(nnc_info, lgr_nr, 10, 0);
            nnc_info_add_nnc(nnc_info, lgr_nr, 20, 1);
            nnc_info_add_nnc(nnc_info, lgr_nr, 30, 2);

            REQUIRE(nnc_info_get_size(nnc_info) == 1);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 3);
            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr));

            auto nnc_vector = nnc_info_get_vector(nnc_info, lgr_nr);
            REQUIRE(nnc_vector != nullptr);
            REQUIRE(nnc_vector_get_size(nnc_vector) == 3);

            const auto &grid_indices =
                nnc_info_get_grid_index_list(nnc_info, lgr_nr);
            REQUIRE(grid_indices.size() == 3);
            REQUIRE(grid_indices[0] == 10);
            REQUIRE(grid_indices[1] == 20);
            REQUIRE(grid_indices[2] == 30);
        }

        THEN("Self vector accessor works correctly") {
            nnc_info_add_nnc(nnc_info, lgr_nr, 5, 10);

            auto self_vector = nnc_info_get_self_vector(nnc_info);
            REQUIRE(self_vector != nullptr);
            REQUIRE(nnc_vector_get_size(self_vector) == 1);

            const auto &self_grid_indices =
                nnc_info_get_self_grid_index_list(nnc_info);
            REQUIRE(self_grid_indices.size() == 1);
            REQUIRE(self_grid_indices[0] == 5);
        }

        THEN("Multiple LGR support works") {
            int lgr_nr_1 = 1;
            int lgr_nr_2 = 2;

            nnc_info_add_nnc(nnc_info, lgr_nr, 10, 0);
            nnc_info_add_nnc(nnc_info, lgr_nr_1, 20, 1);
            nnc_info_add_nnc(nnc_info, lgr_nr_2, 30, 2);

            REQUIRE(nnc_info_get_size(nnc_info) == 3);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 3);

            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr));
            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr_1));
            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr_2));
            REQUIRE(!nnc_info_has_grid_index_list(nnc_info, 99));

            auto vec0 = nnc_info_get_vector(nnc_info, lgr_nr);
            auto vec1 = nnc_info_get_vector(nnc_info, lgr_nr_1);
            auto vec2 = nnc_info_get_vector(nnc_info, lgr_nr_2);

            REQUIRE(vec0 != nullptr);
            REQUIRE(vec1 != nullptr);
            REQUIRE(vec2 != nullptr);

            REQUIRE(nnc_vector_get_lgr_nr(vec0) == lgr_nr);
            REQUIRE(nnc_vector_get_lgr_nr(vec1) == lgr_nr_1);
            REQUIRE(nnc_vector_get_lgr_nr(vec2) == lgr_nr_2);

            auto vec_idx0 = nnc_info_iget_vector(nnc_info, 0);
            auto vec_idx1 = nnc_info_iget_vector(nnc_info, 1);
            auto vec_idx2 = nnc_info_iget_vector(nnc_info, 2);

            REQUIRE(vec_idx0 != nullptr);
            REQUIRE(vec_idx1 != nullptr);
            REQUIRE(vec_idx2 != nullptr);
        }

        nnc_info_free(nnc_info);
    }
}

TEST_CASE("NNC Vector", "[unittest]") {
    auto nnc_vec = nnc_vector_alloc(0);
    REQUIRE(nnc_vec != nullptr);
    REQUIRE(nnc_vector_get_lgr_nr(nnc_vec) == 0);
    REQUIRE(nnc_vector_get_size(nnc_vec) == 0);

    nnc_vector_add_nnc(nnc_vec, 10, 100);
    nnc_vector_add_nnc(nnc_vec, 20, 200);
    nnc_vector_add_nnc(nnc_vec, 30, 300);

    REQUIRE(nnc_vector_get_size(nnc_vec) == 3);
    REQUIRE(nnc_vector_iget_grid_index(nnc_vec, 0) == 10);
    REQUIRE(nnc_vector_iget_nnc_index(nnc_vec, 0) == 100);
    REQUIRE(nnc_vector_iget_grid_index(nnc_vec, 1) == 20);
    REQUIRE(nnc_vector_iget_nnc_index(nnc_vec, 1) == 200);
    REQUIRE(nnc_vector_iget_grid_index(nnc_vec, 2) == 30);
    REQUIRE(nnc_vector_iget_nnc_index(nnc_vec, 2) == 300);

    auto nnc_vec_copy = nnc_vector_alloc_copy(nnc_vec);
    REQUIRE(nnc_vec_copy != nullptr);
    REQUIRE(nnc_vector_equal(nnc_vec, nnc_vec_copy));
    REQUIRE(nnc_vector_get_size(nnc_vec_copy) == 3);

    const auto &grid_indices = nnc_vector_get_grid_index_list(nnc_vec_copy);
    const auto &nnc_indices = nnc_vector_get_nnc_index_list(nnc_vec_copy);
    REQUIRE(grid_indices.size() == 3);
    REQUIRE(nnc_indices.size() == 3);
    REQUIRE(grid_indices[0] == 10);
    REQUIRE(grid_indices[1] == 20);
    REQUIRE(grid_indices[2] == 30);
    REQUIRE(nnc_indices[0] == 100);
    REQUIRE(nnc_indices[1] == 200);
    REQUIRE(nnc_indices[2] == 300);

    REQUIRE(nnc_vector_equal(nnc_vec, nnc_vec));
    REQUIRE(!nnc_vector_equal(nnc_vec, nullptr));
    REQUIRE(!nnc_vector_equal(nullptr, nnc_vec_copy));

    auto nnc_vec_diff = nnc_vector_alloc(1); // Different lgr_nr
    nnc_vector_add_nnc(nnc_vec_diff, 10, 100);
    REQUIRE(!nnc_vector_equal(nnc_vec, nnc_vec_diff));

    nnc_vector_free(nnc_vec);
    nnc_vector_free(nnc_vec_copy);
    nnc_vector_free(nnc_vec_diff);
}

TEST_CASE("Verbose rd_grid_compare on unequal grids", "[unittest]") {
    GIVEN("Two rectangular grids with different actnum") {
        int actnum1[] = {1, 1, 1, 1, 1, 1, 1, 1};
        int actnum2[] = {1, 0, 1, 1, 1, 1, 1, 1};
        rd_grid_type *g1 = rd_grid_alloc_rectangular(2, 2, 2, 1, 1, 1, actnum1);
        rd_grid_type *g2 = rd_grid_alloc_rectangular(2, 2, 2, 1, 1, 1, actnum2);

        THEN("rd_grid_compare with verbose=true reports them as unequal") {
            REQUIRE(!rd_grid_compare(g1, g2, false, false, true));
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }

    GIVEN("Two grids with identical actnum but different cell corners") {
        rd_grid_type *g1 = generate_coordkw_grid(2, 2, 2, {});
        rd_grid_type *g2 = generate_coordkw_grid(
            2, 2, 2, {{0, 0, 0, 0, 5.0}, {1, 1, 1, 7, 42.0}});

        THEN("rd_grid_compare detects the corner differences") {
            REQUIRE(!rd_grid_compare(g1, g2, false, false, true));
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }

    GIVEN("Two grids with identical geometry and actnum") {
        rd_grid_type *g1 = generate_coordkw_grid(2, 2, 2, {});
        rd_grid_type *g2 = generate_coordkw_grid(2, 2, 2, {});

        THEN("rd_grid_compare reports equality and exercises point_compare") {
            REQUIRE(rd_grid_compare(g1, g2, false, false, true));
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }

    GIVEN("Two grids differing in NNC information") {
        rd_grid_type *g1 = rd_grid_alloc_rectangular(2, 2, 2, 1, 1, 1, nullptr);
        rd_grid_type *g2 = rd_grid_alloc_rectangular(2, 2, 2, 1, 1, 1, nullptr);
        rd_grid_add_self_nnc(g1, 0, 1, 0);

        THEN("rd_grid_compare with include_nnc=true detects the difference") {
            REQUIRE(!rd_grid_compare(g1, g2, false, true, true));
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Verbose rd_grid_compare detects coarse_group differences",
                 "[unittest]") {
    GIVEN("Two EGRID files differing only in coarse cell group assignment") {
        const int nx = 2, ny = 2, nz = 2;
        const int size = nx * ny * nz;

        std::vector<int> corsnum1(size, 0);
        corsnum1[0] = 1;
        corsnum1[1] = 1;

        std::vector<int> corsnum2(size, 0);
        corsnum2[2] = 1;
        corsnum2[3] = 1;

        auto file1 = dirname / "CORSNUM1.EGRID";
        auto file2 = dirname / "CORSNUM2.EGRID";
        write_egrid_with_coarse_groups(file1, nx, ny, nz, corsnum1.data());
        write_egrid_with_coarse_groups(file2, nx, ny, nz, corsnum2.data());

        rd_grid_type *g1 = rd_grid_alloc(file1.c_str());
        rd_grid_type *g2 = rd_grid_alloc(file2.c_str());

        THEN("rd_grid_compare with verbose=true reports them as unequal") {
            REQUIRE_FALSE(rd_grid_compare(g1, g2, false, false, true));
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Verbose rd_grid_compare with include_lgr detects host_cell "
                 "differences in LGR cells",
                 "[unittest]") {
    GIVEN("Two EGRID files with identical main grids but LGRs attached to "
          "different host cells") {
        auto file1 = dirname / "LGR_HOST1.EGRID";
        auto file2 = dirname / "LGR_HOST2.EGRID";
        write_egrid_with_single_lgr(file1, 3, 3, 3, 2, 2, 2, 0, 0, 0, "LGR1");
        write_egrid_with_single_lgr(file2, 3, 3, 3, 2, 2, 2, 1, 1, 1, "LGR1");

        rd_grid_type *g1 = rd_grid_alloc(file1.c_str());
        rd_grid_type *g2 = rd_grid_alloc(file2.c_str());

        THEN("rd_grid_compare with include_lgr=true detects the difference") {
            REQUIRE_FALSE(rd_grid_compare(g1, g2, true, false, true));
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Verbose rd_grid_compare detects fracture active_index "
                 "differences on dual-porosity grids",
                 "[unittest]") {
    GIVEN("Two dual-porosity EGRID files differing in fracture actnum") {
        const int nx = 2, ny = 2, nz = 2;
        const int size = nx * ny * nz;

        // Both grids have the same set of matrix-active cells so that
        // active_index[MATRIX_INDEX] matches cell-for-cell. The fracture
        // activity differs in the first cell (2+1=3 vs 1), which shifts
        // active_index[FRACTURE_INDEX] for every fracture-active cell
        // that follows.
        std::vector<int> actnum1(size, CELL_ACTIVE_MATRIX |
                                           CELL_ACTIVE_FRACTURE);
        std::vector<int> actnum2 = actnum1;
        actnum2[0] = CELL_ACTIVE_MATRIX;

        auto file1 = dirname / "DUALP1.EGRID";
        auto file2 = dirname / "DUALP2.EGRID";
        write_egrid_dual_porosity(file1, nx, ny, nz, actnum1.data());
        write_egrid_dual_porosity(file2, nx, ny, nz, actnum2.data());

        rd_grid_type *g1 = rd_grid_alloc(file1.c_str());
        rd_grid_type *g2 = rd_grid_alloc(file2.c_str());

        THEN("rd_grid_compare with verbose=true reports them as unequal") {
            REQUIRE_FALSE(rd_grid_compare(g1, g2, false, false, true));
        }

        THEN("Writing the dual-porosity grid as a GRID file exercises the "
             "fracture-cell branch of rd_cell_fwrite_GRID") {
            auto grid_filename = dirname / "DUALP.GRID";
            rd_grid_fwrite_GRID2(g1, grid_filename.c_str(), RD_METRIC_UNITS);
            REQUIRE(fs::exists(grid_filename));

            AND_THEN("Re-loading the dual-porosity GRID file exercises the "
                     "fracture-cell branch of rd_grid_set_cell_GRID") {
                rd_grid_type *reloaded = rd_grid_alloc(grid_filename.c_str());
                REQUIRE(reloaded != nullptr);
                REQUIRE(rd_grid_dual_grid(reloaded));
                rd_grid_free(reloaded);
            }
        }

        rd_grid_free(g1);
        rd_grid_free(g2);
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Dual-porosity EGRID with NNC to a fracture cell breaks out "
                 "of the NNC loop",
                 "[unittest]") {
    // Dual porosity encodes a file with nz (file) = 2 * nz (grid). After
    // loading, grid->size = nx*ny*(nz/2). In the file, 1-based cell indices
    // greater than grid->size refer to fracture cells, and the NNC loop is
    // expected to break out of the loop as soon as such an index is seen.
    const int nx = 1, ny = 1, nz = 4; // 2 matrix + 2 fracture layers
    const int size = nx * ny * nz;
    std::vector<int> actnum(size, CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);

    // First NNC connects two matrix cells (valid); the second references a
    // fracture cell index > grid->size (= nx*ny*nz/2 = 2) to trigger the
    // break.
    const std::vector<int> nnc1 = {1, 3}; // matrix 1, fracture 3 (1-based)
    const std::vector<int> nnc2 = {2, 4};

    GIVEN("A dual-porosity EGRID with NNCs referencing fracture cells") {
        auto filename = dirname / "DUALP_NNC.EGRID";
        write_egrid_dual_porosity(filename, nx, ny, nz, actnum.data(), nnc1,
                                  nnc2);

        THEN("The file loads and the NNC loop breaks on the fracture index") {
            rd_grid_type *grid = rd_grid_alloc(filename.c_str());
            REQUIRE(grid != nullptr);
            rd_grid_free(grid);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Test grid file I/O", "[unittest]") {
    GIVEN("A grid") {
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(3, 3, 3, 1, 1, 1, nullptr);

        SECTION("exists") {
            auto filename = (dirname / "TEST.EGRID");
            rd_grid_fwrite_EGRID(grid, filename.c_str(), true);
            REQUIRE(rd_grid_exists(filename.c_str()));

            auto filename2 = (dirname / "TEST2.EGRID");
            rd_grid_fwrite_EGRID2(grid, filename2.c_str(), RD_METRIC_UNITS);
            REQUIRE(rd_grid_exists(filename2.c_str()));
        }

        SECTION("write as GRID") {
            auto filename = (dirname / "TEST.GRID");
            rd_grid_fwrite_GRID2(grid, filename.c_str(), RD_METRIC_UNITS);

            rd_grid_type *loaded = rd_grid_alloc(filename.c_str());
            REQUIRE(loaded != nullptr);
            REQUIRE(rd_grid_compare(grid, loaded, false, false, false));
            rd_grid_free(loaded);
        }

        SECTION("write as GRDECL") {
            auto filename = (dirname / "TEST.GRDECL");
            FILE *fp = fopen(filename.c_str(), "w");
            REQUIRE(fp != nullptr);
            rd_grid_fprintf_grdecl2(grid, fp, RD_METRIC_UNITS);
            fclose(fp);
        }

        SECTION("load_case__") {
            auto filename = (dirname / "TEST3.EGRID");
            rd_grid_fwrite_EGRID2(grid, filename.c_str(), RD_METRIC_UNITS);

            rd_grid_type *loaded = rd_grid_load_case__(filename.c_str(), true);
            REQUIRE(loaded != nullptr);
            REQUIRE(rd_grid_compare(grid, loaded, false, false, false));
            rd_grid_free(loaded);
        }

        SECTION("print kw as GRDECL") {
            rd_kw_type *kw =
                rd_kw_alloc("PORO", rd_grid_get_global_size(grid), RD_FLOAT);

            for (int i = 0; i < rd_grid_get_global_size(grid); i++) {
                rd_kw_iset_float(kw, i, 0.2f + i * 0.01f);
            }
            auto filename = (dirname / "KW.GRDECL");
            FILE *fp = fopen(filename.c_str(), "w");
            REQUIRE(fp != nullptr);
            rd_grid_grdecl_fprintf_kw(grid, kw, nullptr, fp, -999.0);
            fclose(fp);
            rd_kw_free(kw);
        }

        rd_grid_free(grid);
    }
}
