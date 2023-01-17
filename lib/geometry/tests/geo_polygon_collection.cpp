#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>

#include <ert/geometry/geo_polygon.hpp>
#include <ert/geometry/geo_polygon_collection.hpp>

void test_create() {
    geo_polygon_collection_type *pc = geo_polygon_collection_alloc();
    test_assert_true(geo_polygon_collection_is_instance(pc));
    test_assert_int_equal(0, geo_polygon_collection_size(pc));
    geo_polygon_collection_free(pc);
}

int main(int argc, char **argv) {
    test_create();
    exit(0);
}
