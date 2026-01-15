#include <catch2/catch.hpp>
#include <ert/geometry/geo_pointset.hpp>

using namespace Catch;

TEST_CASE("geo_pointset_memcpy zeros internal z when not copying zdata",
          "[geometry]") {
    GIVEN("Source and target pointsets with internal z") {
        geo_pointset_type *src = geo_pointset_alloc(true);
        geo_pointset_type *target = geo_pointset_alloc(true);

        // Add some points to source
        geo_pointset_add_xyz(src, 1.0, 2.0, 3.0);
        geo_pointset_add_xyz(src, 4.0, 5.0, 6.0);
        geo_pointset_add_xyz(src, 7.0, 8.0, 9.0);

        // Add different points to target so it has non-zero z values
        geo_pointset_add_xyz(target, 10.0, 11.0, 12.0);

        WHEN("Copying without zdata") {
            geo_pointset_memcpy(src, target, false);

            THEN("Target has same size as source") {
                REQUIRE(geo_pointset_get_size(target) == 3);
            }

            THEN("X and Y coordinates are copied") {
                double x, y;
                geo_pointset_iget_xy(target, 0, &x, &y);
                REQUIRE(x == 1.0);
                REQUIRE(y == 2.0);
                geo_pointset_iget_xy(target, 1, &x, &y);
                REQUIRE(x == 4.0);
                REQUIRE(y == 5.0);
                geo_pointset_iget_xy(target, 2, &x, &y);
                REQUIRE(x == 7.0);
                REQUIRE(y == 8.0);
            }

            THEN("Z coordinates are zeroed") {
                REQUIRE(geo_pointset_iget_z(target, 0) == 0.0);
                REQUIRE(geo_pointset_iget_z(target, 1) == 0.0);
                REQUIRE(geo_pointset_iget_z(target, 2) == 0.0);
            }
        }

        geo_pointset_free(src);
        geo_pointset_free(target);
    }
}
