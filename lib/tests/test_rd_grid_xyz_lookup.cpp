/**
 * Tests for finding the cell that contains a point.
 *
 * rd_grid_get_global_index_from_xyz(grid, x, y, z, start_index) returns the
 * global index of the cell that contains (x, y, z), or -1 if no cell does.
 * A cell "contains" a point when the point lies inside the volume of the
 * cell's hexahedron, or on one of the cell faces that is shared with no
 * neighbour (so points on the outer boundary of the grid still belong to a
 * cell, while points on an internal face belong to exactly one of the two
 * cells that share it).
 *
 * Some special cases:
 * * Cells with degenerate geometry are treated as empty;
 * * A cell with a corner pinned at the origin is considered invalid, so contains no points;
 * * Cells with its "bottom" face lying above its "top" face is also invalid, so contains no points.
 *
 * The optional start_index is a hint where to start the search.
 */

#include <catch2/catch.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>

#include "grid_fixtures.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE("rd_grid_get_global_index_from_xyz on a single unit cell",
          "[unittest]") {
    // Unit cube spanning [1,2]x[1,2]x[0,1]. The cube is offset from the
    // origin so no corner sits at (0, 0, *), which would mark the cell as
    // degenerate and exclude it from containment checks.
    const double corners[8][3] = {
        {1, 1, 0}, {2, 1, 0}, {1, 2, 0}, {2, 2, 0},
        {1, 1, 1}, {2, 1, 1}, {1, 2, 1}, {2, 2, 1},
    };
    auto grid = build_single_cell_grid(corners);

    SECTION("Point at the cell center is inside the cell") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 0.5,
                                                  0) == 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 0.5,
                                                  -1) == 0);
    }

    SECTION("Point far from the cell is outside") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 100.0, 100.0,
                                                  100.0, 0) == -1);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), -100.0, -100.0,
                                                  -100.0, -1) == -1);
    }

    SECTION("Point just above the top of the cell is outside") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 2.0,
                                                  0) == -1);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 2.0,
                                                  -1) == -1);
    }

    SECTION("Point on the outer max-i face is inside the cell") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 2.0, 1.5, 0.5,
                                                  0) == 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 2.0, 1.5, 0.5,
                                                  -1) == 0);
    }

    SECTION("Point on the outer top face is inside the cell") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 1.0,
                                                  0) == 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 1.0,
                                                  -1) == 0);
    }

    SECTION("Point on the outer min-i face is inside the cell") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.0, 1.5, 0.5,
                                                  0) == 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.0, 1.5, 0.5,
                                                  -1) == 0);
    }
}

TEST_CASE("rd_grid_get_global_index_from_xyz on a twisted single cell",
          "[unittest]") {
    // Top and bottom faces swapped so the cell's bottom face lies above its
    // top face. Such a cell is treated as degenerate and contains no
    // points.
    const double corners[8][3] = {
        {1, 1, 1}, {2, 1, 1}, {1, 2, 1}, {2, 2, 1},
        {1, 1, 0}, {2, 1, 0}, {1, 2, 0}, {2, 2, 0},
    };
    auto grid = build_single_cell_grid(corners);

    // 1.5, 1.5, 0.5 is the barycenter, but the cell is "twisted"
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 0.5, 0) ==
            -1);
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 0.5, -1) ==
            -1);
}

TEST_CASE("rd_grid_get_global_index_from_xyz on a tainted single cell",
          "[unittest]") {
    // Corner 0 at (0, 0, 0) marks the cell as degenerate. Such cells are
    // excluded from containment checks even for points that lie inside
    // their nominal volume.
    const double corners[8][3] = {
        {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1},
    };
    auto grid = build_single_cell_grid(corners, 0);

    // The point at (0.5, 0.5, 0.5) is the barycenter,
    // but the cell is "degenerate"
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 0.5, 0.5, 0.5, 0) ==
            -1);
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 0.5, 0.5, 0.5, -1) ==
            -1);
}

TEST_CASE("get_global_index_from_xyz on a concave cell", "[unittest]") {
    // One top corner is pulled inward in the xy-plane, producing a
    // non-convex hexahedron. Containment for such cells must still hold
    // for points solidly inside the cell's bulk.
    const double corners[8][3] = {
        {1, 1, 0}, {2, 1, 0}, {1, 2, 0}, {1.3, 1.3, 0},
        {1, 1, 1}, {2, 1, 1}, {1, 2, 1}, {2, 2, 1},
    };
    auto grid = build_single_cell_grid(corners);

    // Point inside the pull-inward plane
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.2, 1.2, 0.5, 0) ==
            0);
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.2, 1.2, 0.5, -1) ==
            0);

    // Point near the concave notch at the top face
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.8, 1.8, 0.1, 0) ==
            -1);
}

TEST_CASE("get_global_index_from_xyz on a cell with a collapsed edge",
          "[unittest]") {
    // Two of the top corners (6 and 7) are placed at the same point, so the
    // top-j edge of the cell collapses and the cell
    // becomes a wedge.
    const double corners[8][3] = {
        {1, 1, 0}, {2, 1, 0}, {1, 2, 0},   {2, 2, 0},
        {1, 1, 1}, {2, 1, 1}, {1.5, 2, 1}, {1.5, 2, 1},
    };
    auto grid = build_single_cell_grid(corners);

    SECTION("Point on the collapsed edge belongs to the cell") {
        // Midpoint of the segment from corner 2 (1, 2, 0) to the collapsed
        // corners 6/7 at (1.5, 2, 1).
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.25, 2.0, 0.5,
                                                  0) == 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.25, 2.0, 0.5,
                                                  -1) == 0);
    }

    SECTION("Point well inside the wedge is inside the cell") {
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.3, 1.3, 0.5,
                                                  0) == 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.3, 1.3, 0.5,
                                                  -1) == 0);
    }
}

TEST_CASE("rd_grid_get_global_index_from_xyz on a flat single cell",
          "[unittest]") {
    // A cell whose corners all share the same z
    const double corners[8][3] = {
        {1, 1, 0}, {2, 1, 0}, {1, 2, 0}, {2, 2, 0},
        {1, 1, 0}, {2, 1, 0}, {1, 2, 0}, {2, 2, 0},
    };
    auto actnum = GENERATE(0, 1);
    auto grid = build_single_cell_grid(corners, actnum);

    // A point on the flat sheet would be on the cell boundary of a
    // non-degenerate cell, but here the cell has no volume so considered
    // invalid if inactive
    REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 1.5, 0.0, 0) ==
            actnum - 1);
}

TEST_CASE("rd_grid_get_global_index_from_xyz box search on a larger grid",
          "[unittest]") {
    auto grid = make_rectangular_grid(5, 5, 5, 1, 1, 1, nullptr);

    SECTION("Point inside the cell named by the start hint is found there") {
        int cell_100 = rd_grid_get_global_index3(grid.get(), 1, 0, 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 1.5, 0.5, 0.5,
                                                  cell_100) == cell_100);
    }

    SECTION(
        "Point far from the start hint is still found in the correct cell") {
        int cell_040 = rd_grid_get_global_index3(grid.get(), 0, 4, 0);
        int cell_404 = rd_grid_get_global_index3(grid.get(), 4, 0, 4);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 4.5, 0.5, 4.5,
                                                  cell_040) == cell_404);
    }

    SECTION("Point outside the grid is not contained in any cell") {
        int cell_100 = rd_grid_get_global_index3(grid.get(), 1, 0, 0);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 100.0, 100.0,
                                                  100.0, cell_100) == -1);
    }

    SECTION("Point is found without a start hint") {
        int cell_434 = rd_grid_get_global_index3(grid.get(), 4, 3, 4);
        REQUIRE(rd_grid_get_global_index_from_xyz(grid.get(), 4.5, 3.5, 4.5,
                                                  -1) == cell_434);
    }
}
