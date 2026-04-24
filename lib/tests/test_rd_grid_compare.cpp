/**
 * Test rd_grid_compare across many kinds of grids.
 */

#include <catch2/catch.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <vector>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

bool grid_equal(const rd_grid_ptr &g1, const rd_grid_ptr &g2) {
    return rd_grid_compare(g1.get(), g2.get(), true, true, true);
}

TEST_CASE("rd_grid_compare on unequal grids", "[unittest]") {
    GIVEN("Two rectangular grids with different actnum") {
        int actnum1[] = {1, 1, 1, 1, 1, 1, 1, 1};
        int actnum2[] = {1, 0, 1, 1, 1, 1, 1, 1};
        auto g1 = make_rectangular_grid(2, 2, 2, 1, 1, 1, actnum1);
        auto g2 = make_rectangular_grid(2, 2, 2, 1, 1, 1, actnum2);

        THEN("the grids are unequal") { REQUIRE_FALSE(grid_equal(g1, g2)); }
        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }

    GIVEN("Two grids with identical actnum but different cell corners") {
        auto g1 = generate_coordkw_grid(2, 2, 2, {});
        auto g2 = generate_coordkw_grid(
            2, 2, 2, {{0, 0, 0, 0, 5.0}, {1, 1, 1, 7, 42.0}});

        THEN("The grids are unequal") { REQUIRE_FALSE(grid_equal(g1, g2)); }
        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }

    GIVEN("Two grids with identical geometry and actnum") {
        auto g1 = generate_coordkw_grid(2, 2, 2, {});
        auto g2 = generate_coordkw_grid(2, 2, 2, {});

        THEN("the grids are equal") { REQUIRE(grid_equal(g1, g2)); }
        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }

    GIVEN("Two grids differing in NNC information") {
        auto g1 = make_rectangular_grid(2, 2, 2, 1, 1, 1, nullptr);
        auto g2 = make_rectangular_grid(2, 2, 2, 1, 1, 1, nullptr);
        rd_grid_add_self_nnc(g1.get(), 0, 1, 0);

        THEN("The grids are unequal") { REQUIRE_FALSE(grid_equal(g1, g2)); }
        THEN("The grids are equal with include_nnc=false") {
            REQUIRE(rd_grid_compare(g1.get(), g2.get(), true, false, true));
        }
        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_grid_compare detects coarse_group differences",
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
        auto g1 =
            load_egrid_with_coarse_groups(file1, nx, ny, nz, corsnum1.data());
        auto g2 =
            load_egrid_with_coarse_groups(file2, nx, ny, nz, corsnum2.data());

        THEN("The grids are unequal") { REQUIRE_FALSE(grid_equal(g1, g2)); }

        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }

    GIVEN("An EGRID with two coarse groups and another with just one") {
        const int nx = 2, ny = 2, nz = 2;
        const int size = nx * ny * nz;

        std::vector<int> corsnum1(size, 0);
        corsnum1[0] = 1;
        corsnum1[1] = 1;
        corsnum1[2] = 1;
        corsnum1[3] = 1;

        std::vector<int> corsnum2(size, 0);
        corsnum2[0] = 1;
        corsnum2[1] = 1;
        corsnum2[2] = 2;
        corsnum2[3] = 2;

        auto file_two = dirname / "CORSNUM_TWO.EGRID";
        auto file_one = dirname / "CORSNUM_ONE.EGRID";
        auto g1 = load_egrid_with_coarse_groups(file_two, nx, ny, nz,
                                                corsnum2.data());
        auto g2 = load_egrid_with_coarse_groups(file_one, nx, ny, nz,
                                                corsnum1.data());

        THEN("The grids are unequal") { REQUIRE_FALSE(grid_equal(g1, g2)); }

        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "rd_grid_compare with include_lgr detects host_cell "
                 "differences in LGR cells",
                 "[unittest]") {
    GIVEN("Two EGRID files with identical main grids but LGRs attached to "
          "different host cells") {
        auto file1 = dirname / "LGR_HOST1.EGRID";
        auto file2 = dirname / "LGR_HOST2.EGRID";
        auto g1 = load_egrid_with_single_lgr(file1, 3, 3, 3, 2, 2, 2, 0, 0, 0,
                                             "LGR1");
        auto g2 = load_egrid_with_single_lgr(file2, 3, 3, 3, 2, 2, 2, 1, 1, 1,
                                             "LGR1");

        THEN("The grids are not equal") { REQUIRE_FALSE(grid_equal(g1, g2)); }

        THEN("Each grid is equal to itself") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "rd_grid_compare detects fracture active_index "
                 "differences on dual-porosity grids",
                 "[unittest]") {
    GIVEN("Two dual-porosity EGRID files differing in fracture actnum") {
        const int nx = 2, ny = 2, nz = 2;
        const int size = nx * ny * nz;
        std::vector<int> actnum1(size,
                                 CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);
        std::vector<int> actnum2 = actnum1;
        actnum2[0] = CELL_ACTIVE_MATRIX;

        auto file1 = dirname / "DUALP1.EGRID";
        auto file2 = dirname / "DUALP2.EGRID";
        auto g1 = load_egrid_dual_porosity(file1, nx, ny, nz, actnum1.data());
        auto g2 = load_egrid_dual_porosity(file2, nx, ny, nz, actnum2.data());

        THEN("The grids are not equal") { REQUIRE_FALSE(grid_equal(g1, g2)); }

        THEN("The grids are equal to themselves") {
            REQUIRE(grid_equal(g1, g1));
            REQUIRE(grid_equal(g2, g2));
        }

        WHEN("Writing the dual-porosity grid as a GRID") {
            auto grid_filename = dirname / "DUALP.GRID";
            rd_grid_fwrite_GRID2(g1.get(), grid_filename.c_str(),
                                 RD_METRIC_UNITS);
            REQUIRE(fs::exists(grid_filename));
            auto reloaded = read_grid(grid_filename);
            REQUIRE(reloaded != nullptr);
            REQUIRE(rd_grid_dual_grid(reloaded.get()));

            AND_THEN("Reloaded grid is equal to inital grid") {
                REQUIRE(grid_equal(g1, reloaded));
            }
        }
    }
}

TEST_CASE_METHOD(
    Tmpdir,
    "Pairwise rd_grid_compare across a representative collection of grids",
    "[unittest]") {
    auto grids = build_all_grids(dirname);

    for (size_t i = 0; i < grids.size(); ++i) {
        for (size_t j = 0; j < grids.size(); ++j) {
            INFO("comparing " << grids[i].label << " vs " << grids[j].label);
            if (i == j)
                REQUIRE(grid_equal(grids[i].grid, grids[j].grid));
            else
                REQUIRE_FALSE(grid_equal(grids[i].grid, grids[j].grid));
        }
    }
}
