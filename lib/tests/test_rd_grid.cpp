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
 * Arbitrarily chosen depth function.
 */
double depth_function(double x, double y) {
    return cos(3 * x) * sin(2 * y) * sqrt(x * y);
}

/**
 * Given a number of cell with the given width, what
 * is the coordinate of the cell centered at index i?
 */
double center_of_cell(double cell_width, int index) {
    return cell_width * index + cell_width * 0.5;
}

/**
 * Generates a grid with the given number of x, y and z cells
 * all width the given x, y, and z dimensions. depth is set by
 * the depth function applied to the center of the cell.
 */
rd_grid_type *generate_dxv_dyv_dzv_depthz_grid(int num_x, int num_y, int num_z,
                                               double x_width, double y_width,
                                               double z_width) {
    auto x_widths = std::vector<double>(num_x + 1, x_width);
    auto y_widths = std::vector<double>(num_y + 1, y_width);
    auto z_widths = std::vector<double>(num_z, z_width);
    auto z_depths = std::vector<double>((num_x + 1) * (num_y + 1), 0);

    for (int j = 0; j <= num_y; j++) {
        double y = center_of_cell(y_width, j);
        for (int i = 0; i <= num_x; i++) {
            double x = center_of_cell(x_width, i);
            z_depths[i + j * (num_x + 1)] = depth_function(x, y);
        }
    }

    auto grid = rd_grid_alloc_dxv_dyv_dzv_depthz(
        num_x, num_y, num_z, x_widths.data(), y_widths.data(), z_widths.data(),
        z_depths.data(), NULL);
    return grid;
}

TEST_CASE("Test dxv_dyv_dzv grids", "[unittest]") {
    GIVEN("A grid constructed with dxv_dyv_dzv_depthz") {
        int nx = GENERATE(take(3, random(5, 25)));
        int ny = GENERATE(take(3, random(5, 25)));
        int nz = GENERATE(take(3, random(5, 25)));

        double x_width = 1.0 / nx;
        double y_width = 1.0 / ny;
        double z_width = 3.0 / nz;

        rd_grid_type *grid = generate_dxv_dyv_dzv_depthz_grid(
            nx, ny, nz, x_width, y_width, z_width);

        AND_GIVEN("Any cell in that grid") {
            int i = GENERATE_COPY(0, nx - 1, take(3, random(1, nx - 2)));
            int j = GENERATE_COPY(0, ny - 1, take(3, random(1, ny - 2)));
            int k = GENERATE_COPY(0, nz - 1, take(3, random(1, nz - 2)));

            THEN("get_xyz3 should return cell center in x, y plane") {
                double xc, yc, zc;
                rd_grid_get_xyz3(grid, i, j, k, &xc, &yc, &zc);

                REQUIRE_THAT(xc,
                             WithinAbs(center_of_cell(x_width, i), 0.000001));
                REQUIRE_THAT(yc, WithinAbs(center_of_cell(y_width, j), 1e-4));
            }
            THEN("The top cell corner has z offset by depth function") {
                double xc, yc, zc;

                rd_grid_get_cell_corner_xyz3(grid, i, j, k, 0, &xc, &yc, &zc);

                double x_center = center_of_cell(x_width, i);
                double y_center = center_of_cell(y_width, j);
                REQUIRE_THAT(
                    zc,
                    WithinAbs(z_width * k + depth_function(x_center, y_center),
                              1e-4));
            }

            THEN("The bottom cell corner has z offset by depth function") {
                double xc, yc, zc;

                rd_grid_get_cell_corner_xyz3(grid, i, j, k, 4, &xc, &yc, &zc);

                double x_center = center_of_cell(x_width, i);
                double y_center = center_of_cell(y_width, j);
                REQUIRE_THAT(zc,
                             WithinAbs(z_width * (k + 1) +
                                           depth_function(x_center, y_center),
                                       1e-4));
            }

            THEN("The volume should be simple multiplication") {
                double volume = rd_grid_get_cell_volume3(grid, i, j, k);
                REQUIRE_THAT(volume,
                             WithinAbs(z_width * x_width * y_width, 1e-4));
            }
        }
        AND_GIVEN("A grid constructed in the same way") {
            rd_grid_type *grid2 = generate_dxv_dyv_dzv_depthz_grid(
                nx, ny, nz, x_width, y_width, z_width);
            THEN("Those grids are equal") {
                REQUIRE(rd_grid_compare(grid, grid2, false, false, true));
            }
            rd_grid_free(grid2);
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

TEST_CASE("Test no twistedness", "[unittest]") {
    GIVEN("A grid with untwisted cells") {
        rd_grid_type *grid = generate_coordkw_grid(10, 7, 8, {});

        AND_GIVEN("Any cell in that grid") {
            auto i = GENERATE(take(3, random(0, 9)));
            auto j = GENERATE(take(3, random(0, 6)));
            auto k = GENERATE(take(3, random(0, 7)));

            THEN("That cell should have a twist score of 0") {
                REQUIRE(rd_grid_get_cell_twist3(grid, i, j, k) == 0);
            }
        }

        rd_grid_free(grid);
    }
}

TEST_CASE("Test simple twistedness", "[unittest]") {
    GIVEN("A grid with the first cell twisted") {
        auto z_vector = std::vector<std::tuple<int, int, int, int, double>>{
            {0, 0, 0, 0, 0.0},
            {0, 0, 0, 4, -0.25},
        };
        rd_grid_type *grid = generate_coordkw_grid(10, 7, 8, z_vector);

        THEN("That cell should have a twist score of 1") {
            REQUIRE(rd_grid_get_cell_twist3(grid, 0, 0, 0) == 1);
        }

        rd_grid_free(grid);
    }
}

TEST_CASE("Test double twistedness", "[unittest]") {
    GIVEN("A grid with the first cell doubly twisted") {
        auto z_vector = std::vector<std::tuple<int, int, int, int, double>>{
            {0, 0, 0, 0, 0.0},
            {0, 0, 0, 4, -0.25},
            {0, 0, 0, 3, 0.0},
            {0, 0, 0, 7, -0.25},
        };
        rd_grid_type *grid = generate_coordkw_grid(10, 7, 8, z_vector);

        THEN("That cell should have a twist score of 2") {
            REQUIRE(rd_grid_get_cell_twist3(grid, 0, 0, 0) == 2);
        }

        rd_grid_free(grid);
    }
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
