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
