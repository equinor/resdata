#include <catch2/catch.hpp>
#include <ert/ecl/ecl_grid.hpp>
#include <vector>

using namespace Catch;
using namespace Matchers;

TEST_CASE("Test unfractured grids", "[unittest]") {
    GIVEN("An unfractured grid") {
        ecl_grid_type *grid =
            ecl_grid_alloc_rectangular(21, 11, 12, 1, 2, 3, NULL);

        REQUIRE(ecl_grid_get_nactive_fracture(grid) == 0);

        THEN("It should return -1 on any fracture index") {
            auto i = GENERATE(0, 1, 10, 20);
            REQUIRE(ecl_grid_get_active_fracture_index1(grid, i) == -1);
        }

        ecl_grid_free(grid);
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
ecl_grid_type *generate_dxv_dyv_dzv_depthz_grid(int num_x, int num_y, int num_z,
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

    auto grid = ecl_grid_alloc_dxv_dyv_dzv_depthz(
        num_x, num_y, num_z, x_widths.data(), y_widths.data(), z_widths.data(),
        z_depths.data(), NULL);
    return grid;
}

TEST_CASE("Test dxv_dyv_dzv grids", "[unittest]") {
    GIVEN("A grid constructed with dxv_dyv_dzv_depthz") {
        int nx = 100;
        int ny = 100;
        int nz = 10;

        double x_width = 1.0 / nx;
        double y_width = 1.0 / ny;
        double z_width = 3.0 / nz;

        ecl_grid_type *grid = generate_dxv_dyv_dzv_depthz_grid(
            100, 100, 10, x_width, y_width, z_width);

        GIVEN("Any cell in that grid") {
            auto i = GENERATE(take(3, random(0, 99)));
            auto j = GENERATE(take(3, random(0, 99)));
            auto k = GENERATE(take(3, random(0, 9)));

            THEN("get_xyz3 should return cell center in x, y plane") {
                double xc, yc, zc;
                ecl_grid_get_xyz3(grid, i, j, k, &xc, &yc, &zc);

                REQUIRE_THAT(xc,
                             WithinAbs(center_of_cell(x_width, i), 0.000001));
                REQUIRE_THAT(yc, WithinAbs(center_of_cell(y_width, j), 1e-4));
            }
            THEN("The top cell corner has z offset by depth function") {
                double xc, yc, zc;

                ecl_grid_get_cell_corner_xyz3(grid, i, j, k, 0, &xc, &yc, &zc);

                double x_center = center_of_cell(x_width, i);
                double y_center = center_of_cell(y_width, j);
                REQUIRE_THAT(
                    zc,
                    WithinAbs(z_width * k + depth_function(x_center, y_center),
                              1e-4));
            }

            THEN("The bottom cell corner has z offset by depth function") {
                double xc, yc, zc;

                ecl_grid_get_cell_corner_xyz3(grid, i, j, k, 4, &xc, &yc, &zc);

                double x_center = center_of_cell(x_width, i);
                double y_center = center_of_cell(y_width, j);
                REQUIRE_THAT(zc,
                             WithinAbs(z_width * (k + 1) +
                                           depth_function(x_center, y_center),
                                       1e-4));
            }
        }

        ecl_grid_free(grid);
    }
}

TEST_CASE("Test grid compare", "[unittest]") {
    GIVEN("Any two grid constructed the same way") {
        int nx = 100;
        int ny = 100;
        int nz = 10;

        double x_width = 1.0 / nx;
        double y_width = 1.0 / ny;
        double z_width = 3.0 / nz;

        ecl_grid_type *grid1 = generate_dxv_dyv_dzv_depthz_grid(
            100, 100, 10, x_width, y_width, z_width);
        ecl_grid_type *grid2 = generate_dxv_dyv_dzv_depthz_grid(
            100, 100, 10, x_width, y_width, z_width);

        THEN("Those grids are equal") { REQUIRE(grid1); }

        ecl_grid_free(grid1);
        ecl_grid_free(grid2);
    }
}
