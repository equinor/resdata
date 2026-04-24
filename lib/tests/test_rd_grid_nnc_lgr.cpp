#include <catch2/catch.hpp>
#include <algorithm>
#include <memory>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/nnc_info.hpp>
#include <resdata/nnc_vector.hpp>
#include <vector>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE_METHOD(Tmpdir, "Load GRID file with two-element LGR_KW",
                 "[unittest]") {
    GIVEN("A GRID file whose LGR_KW has an empty parent name") {
        auto filename = dirname / "LGR_EMPTY.GRID";
        auto grid = load_grid_file_with_lgr_parent(filename, "LGR1", "");

        THEN("The file loads and has the LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 1);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }

    GIVEN("A GRID file whose LGR_KW declares parent = \"GLOBAL\"") {
        auto filename = dirname / "LGR_GLOBAL.GRID";
        auto grid =
            load_grid_file_with_lgr_parent(filename, "LGR1", GLOBAL_STRING);

        THEN("The file loads and the main grid has the LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 1);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }

    GIVEN("A GRID file with a nested LGR whose parent is another LGR") {
        auto filename = dirname / "LGR_NESTED.GRID";
        auto grid = load_grid_file_with_nested_lgr(filename, "OUTER", "INNER");

        THEN("The file loads and the nested LGR is hosted by the outer LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 2);
            REQUIRE(rd_grid_has_lgr(grid.get(), "OUTER"));
            REQUIRE(rd_grid_has_lgr(grid.get(), "INNER"));
        }
    }
}
TEST_CASE_METHOD(Tmpdir, "Load GRID file with MAPAXES", "[unittest]") {
    GIVEN("A .GRID file with a MAPAXES keyword in the main grid section") {
        const float mapaxes[6] = {
            10.0f, 21.0f, // y-axis end point
            10.0f, 20.0f, // origin
            11.0f, 21.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES.GRID";
        auto grid = load_grid_file_with_mapaxes(filename, mapaxes);

        THEN("The grid loads and reports that mapaxes are in use") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_use_mapaxes(grid.get()));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load GRID file with an even-nz main grid and an LGR",
                 "[unittest]") {
    // When the main grid's nz is even, the dual-porosity heuristic is
    // entered. If an LGR is present, num_corners exceeds nx*ny*nz and the
    // fracture_index is set to nx*ny*nz/2
    GIVEN("A .GRID file with a 1x1x2 main grid and a 1x1x1 LGR") {
        auto filename = dirname / "EVEN_NZ_WITH_LGR.GRID";
        auto grid = load_grid_file_main_with_lgr(filename, 1, 1, 2);

        THEN("The file loads successfully and is not dual-porosity") {
            REQUIRE(grid != nullptr);
            REQUIRE_FALSE(rd_grid_dual_grid(grid.get()));
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Dual-porosity EGRID with NNC is loaded",
                 "[unittest]") {
    const int nx = 1, ny = 1, nz = 4; // 2 matrix + 2 fracture layers
    const int size = nx * ny * nz;
    std::vector<int> actnum(size, CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);

    // First NNC connects two matrix cells (valid); the second references a
    // fracture cell index > grid->size (= nx*ny*nz/2 = 2) which is discarded
    const std::vector<int> nnc1 = {1, 3};
    const std::vector<int> nnc2 = {2, 4};

    GIVEN("A dual-porosity EGRID with NNCs referencing fracture cells") {
        auto filename = dirname / "DUALP_NNC.EGRID";
        auto grid = load_egrid_dual_porosity(filename, nx, ny, nz,
                                             actnum.data(), nnc1, nnc2);
        REQUIRE(grid != nullptr);

        THEN("The first NNC is matrix and the second is fracture") {

            // cell 0 has an NNC
            const nnc_info_type *info0 =
                rd_grid_get_cell_nnc_info1(grid.get(), 0);
            REQUIRE(info0 != nullptr);
            REQUIRE(nnc_info_get_total_size(info0) == 1);
            const nnc_vector_type *self0 = nnc_info_get_self_vector(info0);
            REQUIRE(self0 != nullptr);
            REQUIRE(nnc_vector_iget_grid_index(self0, 0) == 1);

            // Cell 1 has no NNC
            const nnc_info_type *info1 =
                rd_grid_get_cell_nnc_info1(grid.get(), 1);
            REQUIRE(info1 == nullptr);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Fracture-index queries on a dual-porosity grid",
                 "[unittest]") {
    // Dual porosity gives each cell a matrix-copy and a fracture-copy, so
    // the grid maintains a second active-index for fracture-active
    // cells in addition to the matrix-active one.
    const int nx = 2, ny = 2, nz = 2;
    const int size = nx * ny * nz;

    // Mark every cell as both matrix- and fracture-active, except cell 0
    // which is only matrix-active.
    std::vector<int> actnum(size, CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);
    actnum[0] = CELL_ACTIVE_MATRIX;

    auto filename = dirname / "DUALP_FRAC.EGRID";
    auto grid = load_egrid_dual_porosity(filename, nx, ny, nz, actnum.data());

    const int nactive_fracture = rd_grid_get_nactive_fracture(grid.get());

    THEN("The count of fracture-active cells matches the ACTNUM encoding") {
        REQUIRE(nactive_fracture == size - 1);
    }

    THEN("Each fracture-active cell maps to a fracture index") {
        std::vector<bool> seen(nactive_fracture, false);
        for (int g = 0; g < size; ++g) {
            int f = rd_grid_get_active_fracture_index1(grid.get(), g);
            if (actnum[g] & CELL_ACTIVE_FRACTURE) {
                REQUIRE(f >= 0);
                REQUIRE(f < nactive_fracture);
                REQUIRE_FALSE(seen[f]);
                seen[f] = true;
                REQUIRE(rd_grid_get_global_index1F(grid.get(), f) == g);
            } else {
                REQUIRE(f == -1);
            }
        }
        for (bool s : seen)
            REQUIRE(s);
    }

    THEN("A fracture index past the last active returns -1") {
        REQUIRE(rd_grid_get_global_index1F(grid.get(), nactive_fracture) == -1);
    }
}
