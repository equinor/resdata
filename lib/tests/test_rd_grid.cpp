#include <catch2/catch.hpp>
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
