#include <ert/geometry/geo_surface.hpp>
#include <ert/geometry/geo_region.hpp>
#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_pointset.hpp>

#include <catch2/catch.hpp>
#include <memory>

using namespace Catch;

TEST_CASE("geo_pointset_memcpy zeros internal z when not copying zdata",
          "[geometry]") {
    GIVEN("Source and target pointsets with internal z") {
        auto src = make_geo_pointset(true);
        auto target = make_geo_pointset(true);

        geo_pointset_add_xyz(src.get(), 1.0, 2.0, 3.0);
        geo_pointset_add_xyz(src.get(), 4.0, 5.0, 6.0);
        geo_pointset_add_xyz(src.get(), 7.0, 8.0, 9.0);

        geo_pointset_add_xyz(target.get(), 10.0, 11.0, 12.0);

        WHEN("Copying without zdata") {
            geo_pointset_memcpy(src.get(), target.get(), false);

            THEN("Target has same size as source") {
                REQUIRE(geo_pointset_get_size(target.get()) == 3);
            }

            THEN("X and Y coordinates are copied") {
                double x, y;
                geo_pointset_iget_xy(target.get(), 0, &x, &y);
                REQUIRE(x == 1.0);
                REQUIRE(y == 2.0);
                geo_pointset_iget_xy(target.get(), 1, &x, &y);
                REQUIRE(x == 4.0);
                REQUIRE(y == 5.0);
                geo_pointset_iget_xy(target.get(), 2, &x, &y);
                REQUIRE(x == 7.0);
                REQUIRE(y == 8.0);
            }

            THEN("Z coordinates are zeroed") {
                REQUIRE(geo_pointset_iget_z(target.get(), 0) == 0.0);
                REQUIRE(geo_pointset_iget_z(target.get(), 1) == 0.0);
                REQUIRE(geo_pointset_iget_z(target.get(), 2) == 0.0);
            }
        }
    }
}

TEST_CASE("Pointset can be acted on", "[geometry]") {
    GIVEN("A pointset with internal z") {
        auto pointset = make_geo_pointset(true);

        geo_pointset_add_xyz(pointset.get(), 1.0, 2.0, 4.0);
        geo_pointset_add_xyz(pointset.get(), 4.0, 5.0, 9.0);
        geo_pointset_add_xyz(pointset.get(), 7.0, 8.0, 16.0);

        WHEN("Assigning a uniform z value") {
            geo_pointset_assign_z(pointset.get(), 42.0);

            THEN("All z coordinates are set to the value") {
                REQUIRE(geo_pointset_iget_z(pointset.get(), 0) == 42.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 1) == 42.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 2) == 42.0);
            }
        }

        WHEN("Shifting z by a value") {
            geo_pointset_shift_z(pointset.get(), 10.0);

            THEN("All z coordinates are shifted by the value") {
                REQUIRE(geo_pointset_iget_z(pointset.get(), 0) == 14.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 1) == 19.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 2) == 26.0);
            }
        }

        WHEN("Scaling z by a factor") {
            geo_pointset_scale_z(pointset.get(), 2.0);

            THEN("All z coordinates are scaled by the factor") {
                REQUIRE(geo_pointset_iget_z(pointset.get(), 0) == 8.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 1) == 18.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 2) == 32.0);
            }
        }

        WHEN("Taking square root of z coordinates") {
            geo_pointset_isqrt(pointset.get());

            THEN("All z coordinates are square rooted") {
                REQUIRE(geo_pointset_iget_z(pointset.get(), 0) == 2.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 1) == 3.0);
                REQUIRE(geo_pointset_iget_z(pointset.get(), 2) == 4.0);
            }
        }
    }
}

TEST_CASE("geo_polygon segments on a square polygon", "[geometry]") {
    GIVEN("A square polygon") {
        auto polygon = make_geo_polygon("square");
        geo_polygon_add_point(polygon.get(), 0.0, 0.0);
        geo_polygon_add_point(polygon.get(), 10.0, 0.0);
        geo_polygon_add_point(polygon.get(), 10.0, 10.0);
        geo_polygon_add_point(polygon.get(), 0.0, 10.0);
        geo_polygon_close(polygon.get());

        WHEN("Checking a segment that crosses the polygon edge") {
            bool intersects = geo_polygon_segment_intersects(
                polygon.get(), -5.0, 5.0, 5.0, 5.0);

            THEN("Intersection is detected") { REQUIRE(intersects == true); }
        }

        WHEN("Checking a segment that doesn't cross the polygon") {
            bool intersects = geo_polygon_segment_intersects(
                polygon.get(), -5.0, 5.0, -2.0, 5.0);

            THEN("No intersection is detected") {
                REQUIRE(intersects == false);
            }
        }

        WHEN("Getting the length") {
            double length = geo_polygon_get_length(polygon.get());

            THEN("it calculates the perimeter") { REQUIRE(length == 40.0); }
        }
    }
}

TEST_CASE("geo_region index list management", "[geometry]") {
    GIVEN("A pointset with 5 points") {
        auto pointset = make_geo_pointset(true);
        for (int i = 0; i < 5; i++) {
            geo_pointset_add_xyz(pointset.get(), i * 1.0, i * 2.0, i * 3.0);
        }

        WHEN("Creating a region with preselect=true") {
            auto region = make_geo_region(pointset, true);
            const int_vector_type *index_list =
                geo_region_get_index_list(region.get());

            THEN("All points are initially selected") {
                REQUIRE(int_vector_size(index_list) == 5);
            }
        }

        WHEN("Creating a region with preselect=false and resetting") {
            auto region = make_geo_region(pointset, false);
            const int_vector_type *index_list =
                geo_region_get_index_list(region.get());

            THEN("No points are selected after reset") {
                REQUIRE(int_vector_size(index_list) == 0);
            }
        }
    }
}

TEST_CASE("geo_region polygon selection", "[geometry]") {
    GIVEN("A pointset with points inside and outside a polygon") {
        auto pointset = make_geo_pointset(true);
        geo_pointset_add_xyz(pointset.get(), 5.0, 5.0, 0.0);
        geo_pointset_add_xyz(pointset.get(), 2.0, 3.0, 0.0);
        geo_pointset_add_xyz(pointset.get(), 15.0, 15.0, 0.0);
        geo_pointset_add_xyz(pointset.get(), -5.0, 5.0, 0.0);

        auto polygon = make_geo_polygon("square");
        geo_polygon_add_point(polygon.get(), 0.0, 0.0);
        geo_polygon_add_point(polygon.get(), 10.0, 0.0);
        geo_polygon_add_point(polygon.get(), 10.0, 10.0);
        geo_polygon_add_point(polygon.get(), 0.0, 10.0);
        geo_polygon_close(polygon.get());

        WHEN("Selecting inside polygon") {
            auto region = make_geo_region(pointset, false);
            geo_region_select_inside_polygon(region.get(), polygon.get());
            const int_vector_type *index_list =
                geo_region_get_index_list(region.get());

            THEN("Only inside points are selected") {
                REQUIRE(int_vector_size(index_list) == 2);
            }
        }
    }
}

TEST_CASE("geo_surface header equality", "[geometry]") {
    GIVEN("Two surfaces with same dimensions") {
        auto surface1 = make_geo_surface(10, 20, 1.0, 2.0, 0.0, 0.0, 0.0);
        auto surface2 = make_geo_surface(10, 20, 1.0, 2.0, 0.0, 0.0, 0.0);

        WHEN("Comparing headers") {
            bool equal =
                geo_surface_equal_header(surface1.get(), surface2.get());

            THEN("Headers are equal") { REQUIRE(equal == true); }
        }
    }

    GIVEN("Two surfaces with different dimensions") {
        auto surface1 = make_geo_surface(10, 20, 1.0, 2.0, 0.0, 0.0, 0.0);
        auto surface2 = make_geo_surface(15, 20, 1.0, 2.0, 0.0, 0.0, 0.0);

        WHEN("Comparing headers") {
            bool equal =
                geo_surface_equal_header(surface1.get(), surface2.get());

            THEN("Headers are not equal") { REQUIRE(equal == false); }
        }
    }
}

TEST_CASE("geo_surface initialization", "[geometry]") {
    GIVEN("A new surface with dimensions") {
        int nx = 3, ny = 4;
        auto surface = make_geo_surface(nx, ny, 1.0, 2.0, 0.0, 0.0, 0.0);

        WHEN("Checking surface size") {
            int size = geo_surface_get_size(surface.get());

            THEN("Size equals nx * ny") { REQUIRE(size == nx * ny); }

            THEN("Dimensions are correct") {
                REQUIRE(geo_surface_get_nx(surface.get()) == nx);
                REQUIRE(geo_surface_get_ny(surface.get()) == ny);
            }
        }
    }
}
