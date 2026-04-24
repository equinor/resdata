#pragma once

/**
 * Shared test fixtures that build grids
 */

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>

namespace fs = std::filesystem;

/*
 * -----------------------------------------------------------------------
 * writing helpers
 * -----------------------------------------------------------------------
 */

inline void write_empty_kw(const ERT::FortIO &fortio, const char *name) {
    auto kw = make_rd_kw(name, 0, RD_INT);
    rd_kw_fwrite(kw.get(), fortio.get());
}

inline void write_egrid_filehead(const ERT::FortIO &fortio,
                                 int dualp_flag = FILEHEAD_SINGLE_POROSITY) {
    auto filehead = make_rd_kw(FILEHEAD_KW, 100, RD_INT);
    rd_kw_scalar_set_int(filehead.get(), 0);
    rd_kw_iset_int(filehead.get(), FILEHEAD_VERSION_INDEX, 3);
    rd_kw_iset_int(filehead.get(), FILEHEAD_YEAR_INDEX, 2007);
    rd_kw_iset_int(filehead.get(), FILEHEAD_TYPE_INDEX,
                   FILEHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(filehead.get(), FILEHEAD_DUALP_INDEX, dualp_flag);
    rd_kw_iset_int(filehead.get(), FILEHEAD_ORGFORMAT_INDEX,
                   FILEHEAD_ORGTYPE_CORNERPOINT);
    rd_kw_fwrite(filehead.get(), fortio.get());
}

inline void write_egrid_gridhead(const ERT::FortIO &fortio, int gnx, int gny,
                                 int gnz, int grid_nr) {
    auto kw = make_rd_kw(GRIDHEAD_KW, GRIDHEAD_SIZE, RD_INT);
    rd_kw_scalar_set_int(kw.get(), 0);
    rd_kw_iset_int(kw.get(), GRIDHEAD_TYPE_INDEX,
                   GRIDHEAD_GRIDTYPE_CORNERPOINT);
    rd_kw_iset_int(kw.get(), GRIDHEAD_NX_INDEX, gnx);
    rd_kw_iset_int(kw.get(), GRIDHEAD_NY_INDEX, gny);
    rd_kw_iset_int(kw.get(), GRIDHEAD_NZ_INDEX, gnz);
    rd_kw_iset_int(kw.get(), GRIDHEAD_NUMRES_INDEX, 1);
    rd_kw_iset_int(kw.get(), GRIDHEAD_LGR_INDEX, grid_nr);
    rd_kw_fwrite(kw.get(), fortio.get());
}

inline void write_egrid_grid_body(rd_grid_type *grid,
                                  const ERT::FortIO &fortio) {
    auto coord = rd_kw_ptr(rd_grid_alloc_coord_kw(grid), &rd_kw_free);
    rd_kw_fwrite(coord.get(), fortio.get());
    auto zcorn = rd_kw_ptr(rd_grid_alloc_zcorn_kw(grid), &rd_kw_free);
    rd_kw_fwrite(zcorn.get(), fortio.get());
    auto actnum = rd_kw_ptr(rd_grid_alloc_actnum_kw(grid), &rd_kw_free);
    rd_kw_fwrite(actnum.get(), fortio.get());
}

inline void write_int_kw(fortio_type *fortio, const char *name,
                         const int *values, int size) {
    auto kw = make_rd_kw(name, size, RD_INT);
    for (int i = 0; i < size; i++)
        rd_kw_iset_int(kw.get(), i, values[i]);
    rd_kw_fwrite(kw.get(), fortio);
}

inline void write_int_kw(fortio_type *fortio, const char *name,
                         const std::vector<int> &values) {
    write_int_kw(fortio, name, values.data(), static_cast<int>(values.size()));
}

inline void write_float_kw(fortio_type *fortio, const char *name,
                           const float *values, int size) {
    auto kw = make_rd_kw(name, size, RD_FLOAT);
    for (int i = 0; i < size; i++)
        rd_kw_iset_float(kw.get(), i, values[i]);
    rd_kw_fwrite(kw.get(), fortio);
}

inline void write_char8_kw(fortio_type *fortio, const char *name,
                           std::initializer_list<const char *> values) {
    auto kw = make_rd_kw(name, static_cast<int>(values.size()), RD_CHAR);
    int i = 0;
    for (const char *v : values)
        rd_kw_iset_string8(kw.get(), i++, v);
    rd_kw_fwrite(kw.get(), fortio);
}

inline void write_nnc_pair_section(const ERT::FortIO &fortio, int lgr_nr,
                                   const char *first_kw, const char *second_kw,
                                   const std::vector<int> &first,
                                   const std::vector<int> &second) {
    if (first.empty())
        return;
    auto nnchead = make_rd_kw(NNCHEAD_KW, NNCHEAD_SIZE, RD_INT);
    rd_kw_scalar_set_int(nnchead.get(), 0);
    rd_kw_iset_int(nnchead.get(), NNCHEAD_NUMNNC_INDEX,
                   static_cast<int>(first.size()));
    rd_kw_iset_int(nnchead.get(), NNCHEAD_LGR_INDEX, lgr_nr);
    rd_kw_fwrite(nnchead.get(), fortio.get());
    write_int_kw(fortio.get(), first_kw, first);
    write_int_kw(fortio.get(), second_kw, second);
}

/** Unit-cube corners in ZCORN order (z varies slowest). */
inline constexpr float UNIT_CELL_CORNERS[24] = {
    0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
};

inline void write_grid_dimens(const ERT::FortIO &fortio, int nx, int ny,
                              int nz) {
    const int dims[3] = {nx, ny, nz};
    write_int_kw(fortio.get(), DIMENS_KW, dims, 3);
}

inline void write_grid_radial_false(const ERT::FortIO &fortio) {
    write_char8_kw(fortio.get(), RADIAL_KW, {"FALSE"});
}

inline void write_grid_mapunits(const ERT::FortIO &fortio) {
    write_char8_kw(fortio.get(), MAPUNITS_KW, {"METRES"});
}

inline void write_grid_gridunit(const ERT::FortIO &fortio) {
    write_char8_kw(fortio.get(), GRIDUNIT_KW, {"METRES", ""});
}

inline void write_grid_cell(const ERT::FortIO &fortio, int i, int j, int k,
                            int global_index, int host_cell,
                            const float corners[24]) {
    const int coords[7] = {i + 1, j + 1,     k + 1, global_index + 1,
                           1,     host_cell, 0};
    write_int_kw(fortio.get(), COORDS_KW, coords, 7);
    write_float_kw(fortio.get(), CORNERS_KW, corners, 24);
}

inline ERT::FortIO make_fortio_writer(const fs::path &filename,
                                      bool fmt = false) {
    return {filename.c_str(), std::fstream::out, fmt,
            fmt ? false : RD_ENDIAN_FLIP};
}

inline rd_grid_ptr build_grdecl_grid(int nx, int ny, int nz, rd_kw_type *zcorn,
                                     rd_kw_type *coord,
                                     rd_kw_type *actnum = nullptr) {
    return {rd_grid_alloc_GRDECL_kw(nx, ny, nz, zcorn, coord, actnum, nullptr),
            &rd_grid_free};
}

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

/*
 * -----------------------------------------------------------------------
 * Test data generators
 * -----------------------------------------------------------------------
 */

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

/**
 * Writes an EGRID file with a CORSNUM keyword describing coarse
 * cell groups. The grid is an nx*ny*nz rectangular grid with unit cells.
 *
 * corsnum must have nx*ny*nz entries. A value of 0 means the cell is
 * not part of any coarse group; positive values identify (1-based) the
 * coarse group the cell belongs to.
 *
 * actnum, if non-null, must also have nx*ny*nz entries. If null, all
 * cells are treated as active.
 */
inline void write_egrid_with_coarse_groups(const fs::path &filename, int nx,
                                           int ny, int nz, const int *corsnum,
                                           const int *actnum = nullptr) {
    auto grid = make_rectangular_grid(nx, ny, nz, 1.0, 1.0, 1.0, actnum);

    auto fortio = make_fortio_writer(filename);

    write_egrid_filehead(fortio);
    write_egrid_gridhead(fortio, nx, ny, nz, 0);
    write_egrid_grid_body(grid.get(), fortio);
    write_int_kw(fortio.get(), CORSNUM_KW, corsnum, nx * ny * nz);
    write_empty_kw(fortio, ENDGRID_KW);
}

/**
 * Writes a single-grid dual-porosity EGRID file for an nx*ny*nz regular
 * grid. Optionally writes a CORSNUM keyword (coarse cell groups) and/or
 * a NNCHEAD + NNC1 + NNC2 self-NNC section.
 */
inline void write_egrid_dual_porosity(const fs::path &filename, int nx, int ny,
                                      int nz, const int *actnum,
                                      const std::vector<int> &nnc1 = {},
                                      const std::vector<int> &nnc2 = {},
                                      const int *corsnum = nullptr) {
    auto grid = make_rectangular_grid(nx, ny, nz, 1.0, 1.0, 1.0, nullptr);

    auto fortio = make_fortio_writer(filename);

    write_egrid_filehead(fortio, FILEHEAD_DUAL_POROSITY);
    write_egrid_gridhead(fortio, nx, ny, nz, 0);

    auto coord = rd_kw_ptr(rd_grid_alloc_coord_kw(grid.get()), &rd_kw_free);
    rd_kw_fwrite(coord.get(), fortio.get());
    auto zcorn = rd_kw_ptr(rd_grid_alloc_zcorn_kw(grid.get()), &rd_kw_free);
    rd_kw_fwrite(zcorn.get(), fortio.get());

    const int size = nx * ny * nz;
    write_int_kw(fortio.get(), ACTNUM_KW, actnum, size);

    if (corsnum != nullptr)
        write_int_kw(fortio.get(), CORSNUM_KW, corsnum, size);

    write_empty_kw(fortio, ENDGRID_KW);

    write_nnc_pair_section(fortio, 0, NNC1_KW, NNC2_KW, nnc1, nnc2);
}

/**
 * Builds the 24 CORNERS floats for an extruded unit-cube cell at (i,j)
 * with the given z-range [z0, z1], in ZCORN order (z slowest).
 */
inline std::array<float, 24> extruded_unit_corners(int i, int j, float z0,
                                                   float z1) {
    return {{
        (float)i, (float)j,       z0, (float)(i + 1), (float)j,       z0,
        (float)i, (float)(j + 1), z0, (float)(i + 1), (float)(j + 1), z0,
        (float)i, (float)j,       z1, (float)(i + 1), (float)j,       z1,
        (float)i, (float)(j + 1), z1, (float)(i + 1), (float)(j + 1), z1,
    }};
}

/**
 * Specification of a single LGR section inside a .GRID file.
 *
 * If emit_parent is false the LGR_KW is written as a single-element
 * keyword [lgr_name] (no parent specified). If emit_parent is true
 * the LGR_KW has two elements [lgr_name, parent_name]; an empty
 * parent_name, or "GLOBAL", is interpreted as
 * "parent is the main grid", while any other name must refer to a
 * previously defined LGR (nested LGR).
 */
struct LgrGrid {
    std::string lgr_name;
    std::string parent_name = "";
    bool emit_parent = false;
    int nx = 1, ny = 1, nz = 1;
    int host_cell = 1;
};

/**
 * Writes a minimal single-porosity .GRID file with an nx*ny*nz
 * main grid (unit cells, z-range [k, k+1]) and zero or more
 * LGR sections. If mapaxes is non-null a MAPAXES keyword is emitted
 * between MAPUNITS and GRIDUNIT. LGR cells are written as 1x1x1.
 */
inline void write_grid_file_with_lgrs(const fs::path &filename, int nx, int ny,
                                      int nz,
                                      const std::vector<LgrGrid> &lgrs = {},
                                      const float *mapaxes = nullptr) {
    auto fortio = make_fortio_writer(filename);

    write_grid_dimens(fortio, nx, ny, nz);
    write_grid_mapunits(fortio);
    if (mapaxes)
        write_float_kw(fortio.get(), MAPAXES_KW, mapaxes, 6);
    write_grid_gridunit(fortio);
    write_grid_radial_false(fortio);

    int global = 0;
    for (int k = 0; k < nz; k++)
        for (int j = 0; j < ny; j++)
            for (int i = 0; i < nx; i++) {
                auto corners =
                    extruded_unit_corners(i, j, (float)k, (float)(k + 1));
                write_grid_cell(fortio, i, j, k, global++, 0, corners.data());
            }

    for (const auto &lgr : lgrs) {
        if (lgr.emit_parent)
            write_char8_kw(fortio.get(), LGR_KW,
                           {lgr.lgr_name.c_str(), lgr.parent_name.c_str()});
        else
            write_char8_kw(fortio.get(), LGR_KW, {lgr.lgr_name.c_str()});

        write_grid_dimens(fortio, lgr.nx, lgr.ny, lgr.nz);
        write_grid_radial_false(fortio);
        int lgr_global = 0;
        for (int k = 0; k < lgr.nz; k++)
            for (int j = 0; j < lgr.ny; j++)
                for (int i = 0; i < lgr.nx; i++)
                    write_grid_cell(fortio, i, j, k, lgr_global++,
                                    lgr.host_cell, UNIT_CELL_CORNERS);
    }
}

/**
 * Writes a minimal 1x1x1 single-porosity .GRID file (no LGR) that contains
 * a valid MAPAXES keyword in the main grid section.
 */
inline void write_grid_file_with_mapaxes(const fs::path &filename,
                                         const float *mapaxes) {
    auto fortio = make_fortio_writer(filename);

    write_grid_dimens(fortio, 1, 1, 1);
    write_grid_mapunits(fortio);
    write_float_kw(fortio.get(), MAPAXES_KW, mapaxes, 6);
    write_grid_gridunit(fortio);
    write_grid_radial_false(fortio);

    // COORDS-5 (no host-cell field) is the minimal variant accepted
    // when no LGR follows.
    const int coords5[5] = {1, 1, 1, 1, 1};
    write_int_kw(fortio.get(), COORDS_KW, coords5, 5);
    write_float_kw(fortio.get(), CORNERS_KW, UNIT_CELL_CORNERS, 24);
}

/**
 * Writes a minimal formatted .FEGRID file (formatted EGRID) for a 1x1x1
 * single-cell rectangular grid.
 */
inline void write_fegrid_minimal(const fs::path &filename) {
    auto grid = make_rectangular_grid(1, 1, 1, 1.0, 1.0, 1.0, nullptr);

    auto fortio = make_fortio_writer(filename, /*fmt=*/true);

    write_egrid_filehead(fortio);
    write_egrid_gridhead(fortio, 1, 1, 1, 0);
    write_egrid_grid_body(grid.get(), fortio);
    write_empty_kw(fortio, ENDGRID_KW);
}
/**
 * Builds a 1x1x1 grid from 8 arbitrary corner coordinates.
 *
 * Corners are ordered according to EGRID ZCORN:
 *   0: (i_min, j_min, top)     4: (i_min, j_min, bottom)
 *   1: (i_max, j_min, top)     5: (i_max, j_min, bottom)
 *   2: (i_min, j_max, top)     6: (i_min, j_max, bottom)
 *   3: (i_max, j_max, top)     7: (i_max, j_max, bottom)
 */
inline rd_grid_ptr build_single_cell_grid(const double corners[8][3],
                                          int actnum_value = 1) {
    const int nx = 1, ny = 1, nz = 1;
    auto coord_kw = make_rd_kw(COORD_KW, RD_GRID_COORD_SIZE(nx, ny), RD_FLOAT);
    auto zcorn_kw =
        make_rd_kw(ZCORN_KW, RD_GRID_ZCORN_SIZE(nx, ny, nz), RD_FLOAT);
    auto actnum_kw = make_rd_kw(ACTNUM_KW, nx * ny * nz, RD_INT);

    // The four pillars are at (i,j) in {0,1}x{0,1}; pillar layout is
    // 6*(i + j*(nx+1)).
    static const int top_corner[4] = {0, 1, 2, 3};
    static const int bot_corner[4] = {4, 5, 6, 7};
    for (int j = 0; j <= 1; ++j) {
        for (int i = 0; i <= 1; ++i) {
            int pillar = i + j * (nx + 1);
            int top = top_corner[i + 2 * j];
            int bot = bot_corner[i + 2 * j];
            set_pillar(coord_kw.get(), pillar, corners[top][0], corners[top][1],
                       corners[top][2], corners[bot][0], corners[bot][1],
                       corners[bot][2]);
        }
    }

    for (int c = 0; c < 8; ++c) {
        int zi = rd_grid_zcorn_index__(nx, ny, 0, 0, 0, c);
        rd_kw_iset_float(zcorn_kw.get(), zi, corners[c][2]);
    }

    rd_kw_iset_int(actnum_kw.get(), 0, actnum_value);

    return build_grdecl_grid(nx, ny, nz, zcorn_kw.get(), coord_kw.get(),
                             actnum_kw.get());
}

inline rd_grid_ptr load_egrid_with_coarse_groups(const fs::path &filename,
                                                 int nx, int ny, int nz,
                                                 const int *corsnum,
                                                 const int *actnum = nullptr) {
    write_egrid_with_coarse_groups(filename, nx, ny, nz, corsnum, actnum);
    return read_grid(filename);
}

inline rd_grid_ptr load_egrid_dual_porosity(const fs::path &filename, int nx,
                                            int ny, int nz, const int *actnum,
                                            const std::vector<int> &nnc1 = {},
                                            const std::vector<int> &nnc2 = {}) {
    write_egrid_dual_porosity(filename, nx, ny, nz, actnum, nnc1, nnc2);
    return read_grid(filename);
}

inline rd_grid_ptr
load_egrid_dual_porosity_with_coarse_groups(const fs::path &filename, int nx,
                                            int ny, int nz, const int *actnum,
                                            const int *corsnum) {
    write_egrid_dual_porosity(filename, nx, ny, nz, actnum, {}, {}, corsnum);
    return read_grid(filename);
}

/**
 * A minimal Grid containing a 1x1x1 main grid
 * and a 1x1x1 LGR. 
 *
 * A parent_name of "" or "GLOBAL" is treated as "no parent".
 * Any other non-empty value must match an existing grid name (nested LGRs).
 */
inline rd_grid_ptr load_grid_file_with_lgr_parent(const fs::path &filename,
                                                  const char *lgr_name,
                                                  const char *parent_name) {
    write_grid_file_with_lgrs(
        filename, 1, 1, 1, {LgrGrid{lgr_name, parent_name, true, 1, 1, 1, 1}});
    return read_grid(filename);
}

inline rd_grid_ptr load_grid_file_with_mapaxes(const fs::path &filename,
                                               const float *mapaxes) {
    write_grid_file_with_mapaxes(filename, mapaxes);
    return read_grid(filename);
}

inline rd_grid_ptr load_grid_file_main_with_lgr(const fs::path &filename,
                                                int nx, int ny, int nz) {
    write_grid_file_with_lgrs(filename, nx, ny, nz,
                              {LgrGrid{"LGR1", "", false, 1, 1, 1, 1}});
    return read_grid(filename);
}

/**
 * A Grid with a 1x1x1 main grid, and a 1x1x1
 * outer LGR and a 1x1x1 inner LGR whose
 * LGR_KW has two elements so the parent name is set to outer_name.
 */
inline rd_grid_ptr load_grid_file_with_nested_lgr(const fs::path &filename,
                                                  const char *outer_name,
                                                  const char *inner_name) {
    write_grid_file_with_lgrs(
        filename, 1, 1, 1,
        {LgrGrid{outer_name, "", false, 1, 1, 1, 1},
         LgrGrid{inner_name, outer_name, true, 1, 1, 1, 1}});
    return read_grid(filename);
}

inline rd_grid_ptr load_fegrid_minimal(const fs::path &filename) {
    write_fegrid_minimal(filename);
    return read_grid(filename);
}

struct LabeledGrid {
    std::string label;
    rd_grid_ptr grid;
};

inline std::vector<LabeledGrid> build_all_grids(const fs::path &dirname) {
    std::vector<LabeledGrid> grids;

    grids.push_back(
        {"rect-2x2x2", make_rectangular_grid(2, 2, 2, 1, 1, 1, nullptr)});

    {
        int actnum[8] = {1, 1, 1, 1, 0, 1, 1, 1};
        grids.push_back({"rect-2x2x2-inactive",
                         make_rectangular_grid(2, 2, 2, 1, 1, 1, actnum)});
    }

    grids.push_back(
        {"rect-3x3x3", make_rectangular_grid(3, 3, 3, 1, 1, 1, nullptr)});

    grids.push_back({"coordkw-perturbed",
                     generate_coordkw_grid(2, 2, 2, {{1, 1, 1, 7, 42.0}})});

    {
        const int nx = 2, ny = 2, nz = 2;
        std::vector<int> corsnum(nx * ny * nz, 0);
        corsnum[0] = 1;
        corsnum[1] = 1;
        grids.push_back(
            {"egrid-coarse",
             load_egrid_with_coarse_groups(dirname / "ALL_GRIDS_CORSNUM.EGRID",
                                           nx, ny, nz, corsnum.data())});
    }

    {
        const int nx = 2, ny = 2, nz = 2;
        std::vector<int> actnum(nx * ny * nz,
                                CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);
        grids.push_back(
            {"egrid-dual-porosity",
             load_egrid_dual_porosity(dirname / "ALL_GRIDS_DUALP.EGRID", nx, ny,
                                      nz, actnum.data())});
    }

    {
        const int nx = 2, ny = 2, nz = 2;
        const int size = nx * ny * nz;
        std::vector<int> actnum(size,
                                CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);
        std::vector<int> corsnum(size, 0);
        corsnum[0] = 1;
        corsnum[1] = 1;
        grids.push_back({"egrid-dual-porosity-with-coarse",
                         load_egrid_dual_porosity_with_coarse_groups(
                             dirname / "ALL_GRIDS_DUALP_CORS.EGRID", nx, ny, nz,
                             actnum.data(), corsnum.data())});
    }

    grids.push_back(
        {"fegrid-minimal", load_fegrid_minimal(dirname / "ALL_GRIDS.FEGRID")});

    {
        float mapaxes[6] = {1.0f, 11.0f, 1.0f, 1.0f, 11.0f, 1.0f};
        grids.push_back(
            {"grid-mapaxes", load_grid_file_with_mapaxes(
                                 dirname / "ALL_GRIDS_MAPAXES.GRID", mapaxes)});
    }

    grids.push_back(
        {"grid-lgr-parent",
         load_grid_file_with_lgr_parent(dirname / "ALL_GRIDS_LGRPARENT.GRID",
                                        "LGR1", "GLOBAL")});

    grids.push_back({"grid-main-with-lgr",
                     load_grid_file_main_with_lgr(
                         dirname / "ALL_GRIDS_MAIN_LGR.GRID", 1, 1, 2)});

    grids.push_back(
        {"grid-nested-lgr",
         load_grid_file_with_nested_lgr(dirname / "ALL_GRIDS_GRID_NESTED.GRID",
                                        "OUTER", "INNER")});

    return grids;
}
