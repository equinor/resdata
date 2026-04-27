/**
 * Tests for basic usage of the rd_grid.
 *
 * A "regular" grid is a rectangular corner-point grid with axis-aligned unit
 * cells and no additional features (no LGRs, no NNCs, no coarse groups, no
 * dual porosity, no MAPAXES). This is the kind of grid produced by
 * rd_grid_alloc_rectangular().
 */

#include <catch2/catch.hpp>
#include <memory>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <vector>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE("Test unfractured grids", "[unittest]") {
    GIVEN("An unfractured grid") {
        auto grid = make_rectangular_grid(21, 11, 12, 1, 2, 3, NULL);

        REQUIRE(rd_grid_get_nactive_fracture(grid.get()) == 0);

        THEN("It should return -1 on any fracture index") {
            auto i = GENERATE(0, 1, 10, 20);
            REQUIRE(rd_grid_get_active_fracture_index1(grid.get(), i) == -1);
        }
    }
}

TEST_CASE("rd_grid_alloc_GRDECL_kw with explicit ACTNUM", "[unittest]") {
    const int nx = 2, ny = 2, nz = 2;
    auto coord_kw = make_rd_kw(COORD_KW, RD_GRID_COORD_SIZE(nx, ny), RD_FLOAT);
    auto zcorn_kw =
        make_rd_kw(ZCORN_KW, RD_GRID_ZCORN_SIZE(nx, ny, nz), RD_FLOAT);

    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            set_pillar(coord_kw.get(), (i + j * nx), i, j, -1, i, j, -1);
            for (int k = 0; k < nz; k++) {
                for (int c = 0; c < 4; c++) {
                    int zi1 = rd_grid_zcorn_index__(nx, ny, i, j, k, c);
                    int zi2 = rd_grid_zcorn_index__(nx, ny, i, j, k, c + 4);
                    rd_kw_iset_float(zcorn_kw.get(), zi1, k);
                    rd_kw_iset_float(zcorn_kw.get(), zi2, k + 1);
                }
            }
        }
    }

    const int size = nx * ny * nz;
    auto actnum_kw = make_rd_kw(ACTNUM_KW, size, RD_INT);
    rd_kw_scalar_set_int(actnum_kw.get(), 1);
    rd_kw_iset_int(actnum_kw.get(), 0, 0);

    auto grid = rd_grid_ptr(rd_grid_alloc_GRDECL_kw(nx, ny, nz, zcorn_kw.get(),
                                                    coord_kw.get(),
                                                    actnum_kw.get(), NULL),
                            &rd_grid_free);
    REQUIRE(grid != nullptr);
    REQUIRE(rd_grid_get_active_size(grid.get()) == size - 1);
}

TEST_CASE_METHOD(Tmpdir, "Test format writing grid", "[unittest]") {
    GIVEN("A regular Grid") {
        auto rd_grid = make_rectangular_grid(5, 5, 5, 1, 1, 1, nullptr);

        WHEN("Writing that file as FEGRID") {
            auto filename = (dirname / "CASE.FEGRID");
            rd_grid_fwrite_EGRID2(rd_grid.get(), filename.c_str(),
                                  RD_METRIC_UNITS);
            THEN("It is a formatted file") {
                REQUIRE(util_fmt_bit8(filename.c_str()));
            }
            THEN("Reading that grid gives equal grid") {
                auto read = read_grid(filename);

                REQUIRE(rd_grid_compare(rd_grid.get(), read.get(), false, false,
                                        true));
            }
        }

        THEN("Writing that file as a EGRID") {
            auto filename = (dirname / "CASE.EGRID");
            rd_grid_fwrite_EGRID2(rd_grid.get(), filename.c_str(),
                                  RD_METRIC_UNITS);
            THEN("It is an unformatted file") {
                REQUIRE(!util_fmt_bit8((dirname / "CASE.EGRID").c_str()));
            }
            THEN("Reading that grid gives equal grid") {
                auto read = read_grid(filename);
                REQUIRE(rd_grid_compare(rd_grid.get(), read.get(), false, false,
                                        true));
            }
        }
        THEN(
            "Writing that file with unknown extension is an unformatted file") {
            rd_grid_fwrite_EGRID2(rd_grid.get(),
                                  (dirname / "CASE.UNKNOWN").c_str(),
                                  RD_METRIC_UNITS);
            REQUIRE(!util_fmt_bit8((dirname / "CASE.UNKNOWN").c_str()));
        }
    }
}

TEST_CASE("Test utility functions on a regular grid", "[unittest]") {
    GIVEN("A regular grid") {
        int actnum_data[] = {1, 1, 1, 1, 0, 1, 1, 1};
        auto grid = make_rectangular_grid(2, 2, 2, 1.0, 2.0, 3.0, actnum_data);

        SECTION("Grid dimension") {
            REQUIRE(rd_grid_get_nx(grid.get()) == 2);
            REQUIRE(rd_grid_get_ny(grid.get()) == 2);
            REQUIRE(rd_grid_get_nz(grid.get()) == 2);

            int nx, ny, nz, nactive;
            rd_grid_get_dims(grid.get(), &nx, &ny, &nz, &nactive);
            REQUIRE(nx == 2);
            REQUIRE(ny == 2);
            REQUIRE(nz == 2);
            REQUIRE(nactive == 7);

            REQUIRE(rd_grid_get_global_size(grid.get()) == 8);
            REQUIRE(rd_grid_get_nactive(grid.get()) == 7);
            REQUIRE(rd_grid_get_active_size(grid.get()) == 7);
        }

        SECTION("name and unit") {
            const char *name = rd_grid_get_name(grid.get());
            REQUIRE(name == nullptr);

            ert_rd_unit_enum unit = rd_grid_get_unit_system(grid.get());
            REQUIRE(unit == RD_METRIC_UNITS);

            float scale = rd_grid_output_scaling(grid.get(), RD_METRIC_UNITS);
            REQUIRE(scale == 1.0f);
        }

        SECTION("Index") {
            int global_idx = rd_grid_get_global_index3(grid.get(), 0, 0, 0);
            REQUIRE(global_idx == 0);

            int i, j, k;
            rd_grid_get_ijk1(grid.get(), global_idx, &i, &j, &k);
            REQUIRE(i == 0);
            REQUIRE(j == 0);
            REQUIRE(k == 0);

            REQUIRE(rd_grid_ijk_valid(grid.get(), 0, 0, 0));
            REQUIRE(rd_grid_ijk_valid(grid.get(), 1, 1, 1));
            REQUIRE(!rd_grid_ijk_valid(grid.get(), 5, 5, 5));

            int active_idx = rd_grid_get_active_index3(grid.get(), 0, 0, 0);
            REQUIRE(active_idx == 0);

            int active_idx1 = rd_grid_get_active_index1(grid.get(), global_idx);
            REQUIRE(active_idx1 == 0);

            int global_from_active =
                rd_grid_get_global_index1A(grid.get(), active_idx);
            REQUIRE(global_from_active == global_idx);

            rd_grid_get_ijk1A(grid.get(), active_idx, &i, &j, &k);
            REQUIRE(i == 0);
            REQUIRE(j == 0);
            REQUIRE(k == 0);
        }

        SECTION("Cell activity") {
            REQUIRE(rd_grid_cell_active3(grid.get(), 0, 0, 0));
            REQUIRE(rd_grid_cell_active1(grid.get(), 0));
            REQUIRE(!rd_grid_cell_active1(grid.get(), 4));

            REQUIRE(rd_grid_cell_valid1(grid.get(), 0));
            REQUIRE(!rd_grid_cell_invalid1(grid.get(), 0));
        }

        SECTION("Position and geometry") {
            double x, y, z;
            rd_grid_get_xyz3(grid.get(), 0, 0, 0, &x, &y, &z);
            REQUIRE(x == 0.5);
            REQUIRE(y == 1.0);
            REQUIRE(z == 1.5);

            rd_grid_get_xyz1(grid.get(), 1, &x, &y, &z);
            REQUIRE(x == 1.5);
            REQUIRE(y == 1.0);
            REQUIRE(z == 1.5);

            int active_idx = rd_grid_get_active_index1(grid.get(), 6);
            rd_grid_get_xyz1A(grid.get(), active_idx, &x, &y, &z);
            REQUIRE(x == 0.5);
            REQUIRE(y == 3.0);
            REQUIRE(z == 4.5);

            rd_grid_get_corner_xyz(grid.get(), 0, 0, 0, &x, &y, &z);
            REQUIRE(x == 0.0);
            REQUIRE(y == 0.0);
            REQUIRE(z == 0.0);

            rd_grid_get_cell_corner_xyz1(grid.get(), 0, 3, &x, &y, &z);
            REQUIRE(x == 1.0);
            REQUIRE(y == 2.0);
            REQUIRE(z == 0.0);

            double corners_x[8], corners_y[8], corners_z[8];
            rd_grid_export_cell_corners1(grid.get(), 0, corners_x, corners_y,
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
            double dx = rd_grid_get_cell_dx1(grid.get(), 0);
            REQUIRE(dx == 1.0);

            double dy = rd_grid_get_cell_dy1(grid.get(), 0);
            REQUIRE(dy == 2.0);

            double dz = rd_grid_get_cell_dz1(grid.get(), 0);
            REQUIRE(dz == 3.0);

            double thickness = rd_grid_get_cell_thickness1(grid.get(), 0);
            REQUIRE(thickness == 3.0);

            int active_idx = rd_grid_get_active_index1(grid.get(), 0);
            double dx_a = rd_grid_get_cell_dx1A(grid.get(), active_idx);
            REQUIRE(dx_a == 1.0);

            double dy_a = rd_grid_get_cell_dy1A(grid.get(), active_idx);
            REQUIRE(dy_a == 2.0);

            double dz_a = rd_grid_get_cell_dz1A(grid.get(), active_idx);
            REQUIRE(dz_a == 3.0);

            double dx_dist, dy_dist, dz_dist;
            rd_grid_get_distance(grid.get(), 0, 1, &dx_dist, &dy_dist,
                                 &dz_dist);
            REQUIRE(dx_dist == -1.0);
            REQUIRE(dy_dist == 0.0);
            REQUIRE(dz_dist == 0.0);
        }

        SECTION("Volume and depth") {
            double volume = rd_grid_get_cell_volume1(grid.get(), 0);
            REQUIRE(volume == 6.0);

            int active_idx = rd_grid_get_active_index1(grid.get(), 0);
            double volume_a = rd_grid_get_cell_volume1A(grid.get(), active_idx);
            REQUIRE(volume_a == 6.0);

            double cdepth_a = rd_grid_get_cdepth1A(grid.get(), active_idx);
            REQUIRE(cdepth_a == 1.5);

            double top_a = rd_grid_get_top1A(grid.get(), active_idx);
            REQUIRE(top_a == 0.0);

            double cdepth = rd_grid_get_cdepth1(grid.get(), 0);
            REQUIRE(cdepth == 1.5);

            double top2 = rd_grid_get_top2(grid.get(), 0, 0);
            REQUIRE(top2 == 0.0);

            double bottom2 = rd_grid_get_bottom2(grid.get(), 0, 0);
            REQUIRE(bottom2 == 6.0);

            int k_loc = rd_grid_locate_depth(grid.get(), 1.5, 0, 0);
            REQUIRE(k_loc == 0);
        }

        SECTION("Cell containment") {
            double x, y, z;
            rd_grid_get_xyz1(grid.get(), 0, &x, &y, &z);

            REQUIRE(rd_grid_cell_contains_xyz1(grid.get(), 0, x, y, z));

            int found_idx =
                rd_grid_get_global_index_from_xyz(grid.get(), x, y, z, 0);
            REQUIRE(found_idx == 0);

            int i_found, j_found;
            bool found_ij =
                rd_grid_get_ij_from_xy(grid.get(), x, y, 0, &i_found, &j_found);
            REQUIRE(found_ij);
            REQUIRE(i_found == 0);
            REQUIRE(j_found == 0);
        }

        SECTION("Grid properties") {
            REQUIRE(!rd_grid_dual_grid(grid.get()));
            REQUIRE(rd_grid_cell_regular1(grid.get(), 0));
            REQUIRE(!rd_grid_use_mapaxes(grid.get()));
        }

        SECTION("LGR functions") {
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 0);
            REQUIRE(rd_grid_get_lgr_nr(grid.get()) == 0);
            REQUIRE(!rd_grid_has_lgr(grid.get(), "test"));
            REQUIRE(!rd_grid_has_lgr_nr(grid.get(), 1));
            REQUIRE(rd_grid_get_cell_lgr1(grid.get(), 0) == nullptr);
            REQUIRE(rd_grid_test_lgr_consistency(grid.get()));
        }

        SECTION("Coarse cells") {
            REQUIRE(!rd_grid_have_coarse_cells(grid.get()));
            REQUIRE(rd_grid_get_num_coarse_groups(grid.get()) == 0);
            REQUIRE(!rd_grid_cell_in_coarse_group1(grid.get(), 0));
        }

        SECTION("Fractures") {
            REQUIRE(rd_grid_get_nactive_fracture(grid.get()) == 0);
            REQUIRE(rd_grid_get_active_fracture_index1(grid.get(), 0) == -1);
            REQUIRE(rd_grid_get_global_index1F(grid.get(), 0) == -1);
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
            auto zcorn_kw =
                rd_kw_ptr(rd_grid_alloc_zcorn_kw(grid.get()), &rd_kw_free);
            REQUIRE(zcorn_kw != nullptr);
            REQUIRE(rd_kw_get_size(zcorn_kw.get()) ==
                    static_cast<int>(expected_zcorn.size()));
            for (size_t i = 0; i < expected_zcorn.size(); i++)
                REQUIRE(rd_kw_iget_float(zcorn_kw.get(), i) ==
                        expected_zcorn[i]);

            auto actnum_kw =
                rd_kw_ptr(rd_grid_alloc_actnum_kw(grid.get()), &rd_kw_free);
            REQUIRE(actnum_kw != nullptr);
            REQUIRE(rd_kw_get_size(actnum_kw.get()) == 8);
            for (int i = 0; i < 8; i++)
                REQUIRE(rd_kw_iget_int(actnum_kw.get(), i) == actnum_data[i]);

            auto coord_kw =
                rd_kw_ptr(rd_grid_alloc_coord_kw(grid.get()), &rd_kw_free);
            REQUIRE(coord_kw != nullptr);
            REQUIRE(rd_kw_get_size(coord_kw.get()) ==
                    static_cast<int>(expected_coord.size()));
            for (int i = 0; i < rd_kw_get_size(coord_kw.get()); i++)
                REQUIRE(rd_kw_iget_float(coord_kw.get(), i) ==
                        expected_coord[i]);

            auto volume_kw = rd_kw_ptr(
                rd_grid_alloc_volume_kw(grid.get(), false), &rd_kw_free);
            REQUIRE(volume_kw != nullptr);
            REQUIRE(rd_kw_get_size(volume_kw.get()) == 8);
            for (int i = 0; i < rd_kw_get_size(volume_kw.get()); i++)
                REQUIRE(rd_kw_iget_double(volume_kw.get(), i) == 6.0);

            auto volume_kw_active = rd_kw_ptr(
                rd_grid_alloc_volume_kw(grid.get(), true), &rd_kw_free);
            REQUIRE(volume_kw_active != nullptr);
            REQUIRE(rd_kw_get_size(volume_kw_active.get()) == 7);
            for (int i = 0; i < rd_kw_get_size(volume_kw_active.get()); i++)
                REQUIRE(rd_kw_iget_double(volume_kw_active.get(), i) == 6.0);
        }

        SECTION("ZCORN and COORD size/data") {
            int zcorn_size = rd_grid_get_zcorn_size(grid.get());
            REQUIRE(zcorn_size == 2 * 2 * 2 * 8);

            int coord_size = rd_grid_get_coord_size(grid.get());
            REQUIRE(coord_size == static_cast<int>(expected_coord.size()));

            std::vector<float> zcorn_data(zcorn_size);
            rd_grid_init_zcorn_data(grid.get(), zcorn_data.data());
            REQUIRE(zcorn_data == expected_zcorn);

            std::vector<double> zcorn_data_double(zcorn_size);
            rd_grid_init_zcorn_data_double(grid.get(),
                                           zcorn_data_double.data());
            REQUIRE(zcorn_data_double ==
                    std::vector<double>(expected_zcorn.begin(),
                                        expected_zcorn.end()));

            std::vector<float> coord_data(coord_size);
            rd_grid_init_coord_data(grid.get(), coord_data.data());
            REQUIRE(coord_data == expected_coord);

            std::vector<double> coord_data_double(coord_size);
            rd_grid_init_coord_data_double(grid.get(),
                                           coord_data_double.data());
            REQUIRE(coord_data_double ==
                    std::vector<double>(expected_coord.begin(),
                                        expected_coord.end()));

            std::vector<int> actnum(rd_grid_get_global_size(grid.get()));
            rd_grid_init_actnum_data(grid.get(), actnum.data());
            for (int i = 0; i < 8; i++)
                REQUIRE(actnum[i] == actnum_data[i]);
        }

        SECTION("copy & reset actnum") {
            auto copy =
                rd_grid_ptr(rd_grid_alloc_copy(grid.get()), &rd_grid_free);
            REQUIRE(copy != nullptr);
            REQUIRE(
                rd_grid_compare(grid.get(), copy.get(), false, false, false));

            std::vector<int> new_actnum(8, 1);
            new_actnum[0] = 0;
            new_actnum[1] = 0;
            rd_grid_reset_actnum(copy.get(), new_actnum.data());

            REQUIRE(rd_grid_get_nactive(copy.get()) == 6);
        }

        SECTION("Grid copy with NNCs") {
            rd_grid_add_self_nnc(grid.get(), 0, 1, 0);
            rd_grid_add_self_nnc(grid.get(), 0, 2, 1);
            rd_grid_add_self_nnc(grid.get(), 1, 3, 2);

            auto copy =
                rd_grid_ptr(rd_grid_alloc_copy(grid.get()), &rd_grid_free);
            REQUIRE(copy != nullptr);

            auto nnc_info_orig = rd_grid_get_cell_nnc_info1(grid.get(), 0);
            auto nnc_info_copy = rd_grid_get_cell_nnc_info1(copy.get(), 0);
            REQUIRE(nnc_info_copy != nullptr);
            REQUIRE(nnc_info_equal(nnc_info_orig, nnc_info_copy));

            auto nnc_info_orig1 = rd_grid_get_cell_nnc_info1(grid.get(), 1);
            auto nnc_info_copy1 = rd_grid_get_cell_nnc_info1(copy.get(), 1);
            REQUIRE(nnc_info_copy1 != nullptr);
            REQUIRE(nnc_info_equal(nnc_info_orig1, nnc_info_copy1));
        }

        SECTION("Export") {
            int num_cells = rd_grid_get_global_size(grid.get());
            std::vector<int> global_index(num_cells);
            for (int i = 0; i < num_cells; i++) {
                global_index[i] = i;
            }

            std::vector<int> index_data(num_cells * 4);
            rd_grid_export_index(grid.get(), global_index.data(),
                                 index_data.data(), false);
            REQUIRE(index_data == std::vector<int>{
                                      0, 0, 0, 0, 1, 0, 0, 1, 0,  1, 0,
                                      2, 1, 1, 0, 3, 0, 0, 1, -1, 1, 0,
                                      1, 4, 0, 1, 1, 5, 1, 1, 1,  6,
                                  });

            std::vector<double> volume_output(num_cells);
            rd_grid_export_volume(grid.get(), num_cells, global_index.data(),
                                  volume_output.data());
            REQUIRE(volume_output.size() == 8);
            for (auto v : volume_output)
                REQUIRE(v == 6.0);

            std::vector<double> position_output(num_cells * 3);
            rd_grid_export_position(grid.get(), num_cells, global_index.data(),
                                    position_output.data());
            REQUIRE(position_output == std::vector<double>{
                                           0.5, 1.0, 1.5, 1.5, 1.0, 1.5,
                                           0.5, 3.0, 1.5, 1.5, 3.0, 1.5,
                                           0.5, 1.0, 4.5, 1.5, 1.0, 4.5,
                                           0.5, 3.0, 4.5, 1.5, 3.0, 4.5,
                                       });

            std::vector<double> corners_output(num_cells * 24);
            export_corners(grid.get(), num_cells, global_index.data(),
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
            auto nnc_info_before = rd_grid_get_cell_nnc_info1(grid.get(), 0);
            REQUIRE(nnc_info_before == nullptr);

            rd_grid_add_self_nnc(grid.get(), 0, 1, 0);

            auto nnc_info = rd_grid_get_cell_nnc_info1(grid.get(), 0);
            REQUIRE(nnc_info != nullptr);
            REQUIRE(nnc_info_get_size(nnc_info) == 1);

            auto nnc_vector = nnc_info_get_self_vector(nnc_info);
            REQUIRE(nnc_vector != nullptr);
            REQUIRE(nnc_vector_get_size(nnc_vector) == 1);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 0) == 1);
            REQUIRE(nnc_vector_iget_nnc_index(nnc_vector, 0) == 0);

            REQUIRE(nnc_info_get_lgr_nr(nnc_info) == 0);
            REQUIRE(nnc_vector_get_lgr_nr(nnc_vector) == 0);

            rd_grid_add_self_nnc(grid.get(), 0, 2, 1);
            rd_grid_add_self_nnc(grid.get(), 0, 3, 2);

            nnc_info = rd_grid_get_cell_nnc_info1(grid.get(), 0);
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

            rd_grid_add_self_nnc(grid.get(), 1, 3, 3);
            rd_grid_add_self_nnc(grid.get(), 1, 5, 4);

            auto nnc_info_cell1 = rd_grid_get_cell_nnc_info1(grid.get(), 1);
            REQUIRE(nnc_info_cell1 != nullptr);
            REQUIRE(nnc_info_get_total_size(nnc_info_cell1) == 2);

            auto nnc_vector_cell1 = nnc_info_get_self_vector(nnc_info_cell1);
            REQUIRE(nnc_vector_get_size(nnc_vector_cell1) == 2);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector_cell1, 0) == 3);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector_cell1, 1) == 5);

            // cells without NNCs return nullptr
            REQUIRE(rd_grid_get_cell_nnc_info1(grid.get(), 4) == nullptr);
            REQUIRE(rd_grid_get_cell_nnc_info1(grid.get(), 6) == nullptr);

            REQUIRE(nnc_info_has_grid_index_list(nnc_info, 0));
            REQUIRE(!nnc_info_has_grid_index_list(nnc_info, 1));
        }

        SECTION("NNC copy and equality") {
            using nnc_info_ptr =
                std::unique_ptr<nnc_info_type, decltype(&nnc_info_free)>;

            rd_grid_add_self_nnc(grid.get(), 0, 1, 0);
            rd_grid_add_self_nnc(grid.get(), 0, 2, 1);

            auto nnc_info_orig = rd_grid_get_cell_nnc_info1(grid.get(), 0);
            REQUIRE(nnc_info_orig != nullptr);

            auto nnc_info_copy = nnc_info_ptr(
                nnc_info_alloc_copy(nnc_info_orig), &nnc_info_free);
            REQUIRE(nnc_info_copy != nullptr);
            REQUIRE(nnc_info_equal(nnc_info_orig, nnc_info_copy.get()));

            REQUIRE(nnc_info_get_lgr_nr(nnc_info_copy.get()) ==
                    nnc_info_get_lgr_nr(nnc_info_orig));
            REQUIRE(nnc_info_get_size(nnc_info_copy.get()) ==
                    nnc_info_get_size(nnc_info_orig));
            REQUIRE(nnc_info_get_total_size(nnc_info_copy.get()) ==
                    nnc_info_get_total_size(nnc_info_orig));

            auto nnc_vector_orig = nnc_info_get_self_vector(nnc_info_orig);
            auto nnc_vector_copy =
                nnc_info_get_self_vector(nnc_info_copy.get());
            REQUIRE(nnc_vector_get_size(nnc_vector_copy) ==
                    nnc_vector_get_size(nnc_vector_orig));
            REQUIRE(nnc_vector_equal(nnc_vector_orig, nnc_vector_copy));

            REQUIRE(nnc_info_equal(nnc_info_orig, nnc_info_copy.get()));
            REQUIRE(!nnc_info_equal(nnc_info_orig, nullptr));
            REQUIRE(!nnc_info_equal(nullptr, nnc_info_copy.get()));
        }

        SECTION("NNC with inactive cells") {
            // Add NNC involving the inactive cell 4
            rd_grid_add_self_nnc(grid.get(), 3, 4, 0);

            auto nnc_info = rd_grid_get_cell_nnc_info1(grid.get(), 3);
            auto nnc_vector = nnc_info_get_self_vector(nnc_info);

            REQUIRE(nnc_vector_get_size(nnc_vector) == 1);
            REQUIRE(nnc_vector_iget_grid_index(nnc_vector, 0) == 4);

            // Verify the inactive cell can also have NNC info
            rd_grid_add_self_nnc(grid.get(), 4, 5, 1);
            auto nnc_info_inactive = rd_grid_get_cell_nnc_info1(grid.get(), 4);
            REQUIRE(nnc_info_inactive != nullptr);
        }

        GIVEN("A grid keyword") {
            auto kw =
                make_rd_kw("PORO", rd_grid_get_nactive(grid.get()), RD_FLOAT);

            for (int i = 0; i < rd_grid_get_nactive(grid.get()); i++) {
                rd_kw_iset_float(kw.get(), i, 0.2f + i * 0.01f);
            }

            SECTION("column property") {
                double prop =
                    rd_grid_get_property(grid.get(), kw.get(), 0, 0, 0);
                REQUIRE(prop >= 0.0);

                auto column = make_double_vector(0, 0.0);
                rd_grid_get_column_property(grid.get(), kw.get(), 0, 0,
                                            column.get());
                REQUIRE(double_vector_size(column.get()) > 0);
            }

            SECTION("Keyword copy") {
                auto target_kw = make_rd_kw(
                    "TARGET", rd_grid_get_global_size(grid.get()), RD_FLOAT);
                rd_grid_global_kw_copy(grid.get(), target_kw.get(), kw.get());

                auto compressed_kw = make_rd_kw(
                    "COMP", rd_grid_get_active_size(grid.get()), RD_FLOAT);
                rd_grid_compressed_kw_copy(grid.get(), compressed_kw.get(),
                                           target_kw.get());
            }

            SECTION("Export data functions") {
                std::vector<int> global_index = {0, 1, 2};
                std::vector<int> int_output(3);

                auto int_kw = make_rd_kw(
                    "PVTNUM", rd_grid_get_global_size(grid.get()), RD_INT);
                for (int i = 0; i < rd_grid_get_global_size(grid.get()); i++) {
                    rd_kw_iset_int(int_kw.get(), i, 1);
                }
                rd_grid_export_data_as_int(3, global_index.data(), int_kw.get(),
                                           int_output.data());

                REQUIRE(int_output[0] == 1);
                REQUIRE(int_output[1] == 1);
                REQUIRE(int_output[2] == 1);

                std::vector<double> double_output(3);
                rd_grid_export_data_as_double(3, global_index.data(), kw.get(),
                                              double_output.data());

                REQUIRE_THAT(double_output[0],
                             Catch::Matchers::WithinAbs(0.20, 0.0001));
                REQUIRE_THAT(double_output[1],
                             Catch::Matchers::WithinAbs(0.21, 0.0001));
                REQUIRE_THAT(double_output[2],
                             Catch::Matchers::WithinAbs(0.22, 0.0001));
            }
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_grid_load_case", "[unittest]") {
    // rd_grid_load_case accepts a variety of path-like inputs and tries to
    // resolve them to a grid file. To distinguish which file is
    // actually loaded we populate the temporary directory with grids of
    // distinct dimensions and compare the loaded grid to the expected one.
    auto grid_5x5x5 = make_rectangular_grid(5, 5, 5, 1, 1, 1, nullptr);
    auto grid_2x2x2 = make_rectangular_grid(2, 2, 2, 1, 1, 1, nullptr);
    auto grid_3x3x3 = make_rectangular_grid(3, 3, 3, 1, 1, 1, nullptr);

    auto case_basename = dirname / "CASE";
    auto egrid_path = dirname / "CASE.EGRID";
    auto grid_path = dirname / "CASE.GRID";
    auto only_grid_base = dirname / "ONLYGRID";
    auto only_grid_path = dirname / "ONLYGRID.GRID";
    auto fegrid_base = dirname / "ONLYFEGRID";
    auto fegrid_path = dirname / "ONLYFEGRID.FEGRID";

    rd_grid_fwrite_EGRID2(grid_5x5x5.get(), egrid_path.c_str(),
                          RD_METRIC_UNITS);
    rd_grid_fwrite_GRID2(grid_2x2x2.get(), grid_path.c_str(), RD_METRIC_UNITS);
    rd_grid_fwrite_GRID2(grid_3x3x3.get(), only_grid_path.c_str(),
                         RD_METRIC_UNITS);
    write_fegrid_minimal(fegrid_path);
    auto read_fegrid = read_grid(fegrid_path);

    auto load_and_compare = [](const fs::path &case_path,
                               rd_grid_type *expected) {
        auto loaded =
            rd_grid_ptr(rd_grid_load_case(case_path.c_str()), &rd_grid_free);
        REQUIRE(loaded != nullptr);
        REQUIRE(rd_grid_compare(loaded.get(), expected, true, true, true));
    };

    GIVEN("A path pointing directly to an existing .EGRID file") {
        THEN("the grid contained in that file is loaded") {
            load_and_compare(egrid_path, grid_5x5x5.get());
        }
    }

    GIVEN("A path pointing directly to an existing .GRID file") {
        THEN("the grid contained in that file is loaded") {
            load_and_compare(grid_path, grid_2x2x2.get());
        }
    }

    GIVEN("A basename with both an .EGRID and a .GRID on disc") {
        THEN("the .EGRID is preferred over the .GRID") {
            load_and_compare(case_basename, grid_5x5x5.get());
        }
    }

    GIVEN("A path to a non-grid file sharing the basename with grid files") {
        auto data_path = dirname / "CASE.DATA";
        {
            std::ofstream ofs(data_path);
            ofs << "RUNSPEC\n";
        }
        THEN("the basename is extracted and the matching grid is loaded") {
            load_and_compare(data_path, grid_5x5x5.get());
        }
    }

    GIVEN("A path to an unformatted non-grid file sharing the basename with "
          "grid files") {
        auto unrst_path = dirname / "CASE.UNRST";
        {
            std::ofstream ofs(unrst_path);
            ofs << "restart stub";
        }
        THEN("the matching unformatted grid is loaded") {
            load_and_compare(unrst_path, grid_5x5x5.get());
        }
    }

    GIVEN("A path that does not correspond to any grid on disc") {
        auto missing_path = dirname / "NOPE";
        THEN("no grid is returned") {
            REQUIRE(rd_grid_load_case(missing_path.c_str()) == nullptr);
        }

        AND_GIVEN("a path with a grid-like extension that does not exist") {
            auto missing_egrid = dirname / "DOES_NOT_EXIST.EGRID";
            THEN("no grid is returned") {
                REQUIRE(rd_grid_load_case(missing_egrid.c_str()) == nullptr);
            }
        }
    }

    GIVEN("A basename for which only a .GRID file exists") {
        THEN("the .GRID file is loaded") {
            load_and_compare(only_grid_base, grid_3x3x3.get());
        }

        AND_GIVEN("an unformatted non-grid file alongside the .GRID") {
            auto unrst_path = dirname / "ONLYGRID.UNRST";
            {
                std::ofstream ofs(unrst_path);
                ofs << "restart stub";
            }
            THEN("the .GRID file is still loaded") {
                load_and_compare(unrst_path, grid_3x3x3.get());
            }
        }
    }

    GIVEN("A basename for which only a formatted .FEGRID file exists") {
        THEN("the .FEGRID file is loaded") {
            load_and_compare(fegrid_base, read_fegrid.get());
        }
    }
}
