#include <ert/geometry/geo_region.hpp>
#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_pointset.hpp>

#include <catch2/catch.hpp>

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

TEST_CASE("Pointset can be acted on", "[geometry]") {
    GIVEN("A pointset with internal z") {
        geo_pointset_type *pointset = geo_pointset_alloc(true);

        // Add some points with different z values
        geo_pointset_add_xyz(pointset, 1.0, 2.0, 4.0);
        geo_pointset_add_xyz(pointset, 4.0, 5.0, 9.0);
        geo_pointset_add_xyz(pointset, 7.0, 8.0, 16.0);

        WHEN("Assigning a uniform z value") {
            geo_pointset_assign_z(pointset, 42.0);

            THEN("All z coordinates are set to the value") {
                REQUIRE(geo_pointset_iget_z(pointset, 0) == 42.0);
                REQUIRE(geo_pointset_iget_z(pointset, 1) == 42.0);
                REQUIRE(geo_pointset_iget_z(pointset, 2) == 42.0);
            }
        }

        WHEN("Shifting z by a value") {
            geo_pointset_shift_z(pointset, 10.0);

            THEN("All z coordinates are shifted by the value") {
                REQUIRE(geo_pointset_iget_z(pointset, 0) == 14.0);
                REQUIRE(geo_pointset_iget_z(pointset, 1) == 19.0);
                REQUIRE(geo_pointset_iget_z(pointset, 2) == 26.0);
            }
        }

        WHEN("Scaling z by a factor") {
            geo_pointset_scale_z(pointset, 2.0);

            THEN("All z coordinates are scaled by the factor") {
                REQUIRE(geo_pointset_iget_z(pointset, 0) == 8.0);
                REQUIRE(geo_pointset_iget_z(pointset, 1) == 18.0);
                REQUIRE(geo_pointset_iget_z(pointset, 2) == 32.0);
            }
        }

        WHEN("Taking square root of z coordinates") {
            geo_pointset_isqrt(pointset);

            THEN("All z coordinates are square rooted") {
                REQUIRE(geo_pointset_iget_z(pointset, 0) == 2.0);
                REQUIRE(geo_pointset_iget_z(pointset, 1) == 3.0);
                REQUIRE(geo_pointset_iget_z(pointset, 2) == 4.0);
            }
        }

        geo_pointset_free(pointset);
    }
}

TEST_CASE("geo_polygon segments on a square polygon", "[geometry]") {
    GIVEN("A square polygon") {
        geo_polygon_type *polygon = geo_polygon_alloc("square");
        geo_polygon_add_point(polygon, 0.0, 0.0);
        geo_polygon_add_point(polygon, 10.0, 0.0);
        geo_polygon_add_point(polygon, 10.0, 10.0);
        geo_polygon_add_point(polygon, 0.0, 10.0);
        geo_polygon_close(polygon);

        WHEN("Checking a segment that crosses the polygon edge") {
            bool intersects =
                geo_polygon_segment_intersects(polygon, -5.0, 5.0, 5.0, 5.0);

            THEN("Intersection is detected") { REQUIRE(intersects == true); }
        }

        WHEN("Checking a segment that doesn't cross the polygon") {
            bool intersects =
                geo_polygon_segment_intersects(polygon, -5.0, 5.0, -2.0, 5.0);

            THEN("No intersection is detected") {
                REQUIRE(intersects == false);
            }
        }

        WHEN("Getting the length") {
            double length = geo_polygon_get_length(polygon);

            THEN("it calculates the perimeter") { REQUIRE(length == 40.0); }
        }

        geo_polygon_free(polygon);
    }
}

TEST_CASE("geo_region index list management", "[geometry]") {
    GIVEN("A pointset with 5 points") {
        geo_pointset_type *pointset = geo_pointset_alloc(true);
        for (int i = 0; i < 5; i++) {
            geo_pointset_add_xyz(pointset, i * 1.0, i * 2.0, i * 3.0);
        }

        WHEN("Creating a region with preselect=true") {
            geo_region_type *region = geo_region_alloc(pointset, true);
            const int_vector_type *index_list =
                geo_region_get_index_list(region);

            THEN("All points are initially selected") {
                REQUIRE(int_vector_size(index_list) == 5);
            }

            geo_region_free(region);
        }

        WHEN("Creating a region with preselect=false and resetting") {
            geo_region_type *region = geo_region_alloc(pointset, false);
            geo_region_reset(region);
            const int_vector_type *index_list =
                geo_region_get_index_list(region);

            THEN("No points are selected after reset") {
                REQUIRE(int_vector_size(index_list) == 0);
            }

            geo_region_free(region);
        }

        geo_pointset_free(pointset);
    }
}

TEST_CASE("geo_region polygon selection", "[geometry]") {
    GIVEN("A pointset with points inside and outside a polygon") {
        geo_pointset_type *pointset = geo_pointset_alloc(true);
        // Points inside square (0,0) to (10,10)
        geo_pointset_add_xyz(pointset, 5.0, 5.0, 0.0);
        geo_pointset_add_xyz(pointset, 2.0, 3.0, 0.0);
        // Points outside square
        geo_pointset_add_xyz(pointset, 15.0, 15.0, 0.0);
        geo_pointset_add_xyz(pointset, -5.0, 5.0, 0.0);

        geo_polygon_type *polygon = geo_polygon_alloc("square");
        geo_polygon_add_point(polygon, 0.0, 0.0);
        geo_polygon_add_point(polygon, 10.0, 0.0);
        geo_polygon_add_point(polygon, 10.0, 10.0);
        geo_polygon_add_point(polygon, 0.0, 10.0);
        geo_polygon_close(polygon);

        WHEN("Selecting inside polygon") {
            geo_region_type *region = geo_region_alloc(pointset, false);
            geo_region_select_inside_polygon(region, polygon);
            const int_vector_type *index_list =
                geo_region_get_index_list(region);

            THEN("Only inside points are selected") {
                REQUIRE(int_vector_size(index_list) == 2);
            }

            geo_region_free(region);
        }

        geo_polygon_free(polygon);
        geo_pointset_free(pointset);
    }
}
