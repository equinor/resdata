#include <catch2/catch.hpp>
#include <ert/ecl/ecl_grid.hpp>

TEST_CASE("Test unfractured grids", "[unittest]") {
    GIVEN("An unfractured grid") {
        ecl_grid_type* grid =
            ecl_grid_alloc_rectangular(21, 11, 12, 1, 2, 3, NULL);

        REQUIRE(ecl_grid_get_nactive_fracture(grid) == 0);

        THEN("It should return -1 on any fracture index") {
            auto i = 1;
            REQUIRE(ecl_grid_get_active_fracture_index1(grid, i) == -1);
        }

        ecl_grid_free(grid);
    }
}
