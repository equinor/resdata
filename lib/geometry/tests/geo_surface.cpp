#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <ert/geometry/geo_surface.hpp>

void test_load(const char *input_file, const char *broken_file) {
    geo_surface_type *surface = geo_surface_fload_alloc_irap(input_file, false);
    double *data =
        (double *)util_calloc(geo_surface_get_size(surface), sizeof *data);

    test_assert_true(geo_surface_fload_irap_zcoord(surface, input_file, data));
    test_assert_false(
        geo_surface_fload_irap_zcoord(surface, "/does/not/exist", data));
    test_assert_false(
        geo_surface_fload_irap_zcoord(surface, broken_file, data));

    free(data);
    geo_surface_free(surface);
}

void test_fprintf(const char *input_file) {
    geo_surface_type *surface1 = geo_surface_fload_alloc_irap(input_file, true);
    test_work_area_type *work_area = test_work_area_alloc("SURFACE-FPRINTF");

    geo_surface_fprintf_irap(surface1, "surface/test/surface.irap");
    {
        geo_surface_type *surface2 =
            geo_surface_fload_alloc_irap("surface/test/surface.irap", true);

        test_assert_true(geo_surface_equal(surface1, surface2));

        geo_surface_free(surface2);
    }
    test_work_area_free(work_area);
    geo_surface_free(surface1);
}

void test_create_new(const char *input_file) {
    geo_surface_type *surface =
        geo_surface_alloc_new(260, 511, 50.0, 50.0, 444230.0, 6809537.0, -30.0);
    test_assert_true(geo_surface_get_nx(surface) == 260);
    test_assert_true(geo_surface_get_ny(surface) == 511);
    test_assert_true(geo_surface_get_size(surface) == 511 * 260);

    geo_surface_type *surirap = geo_surface_fload_alloc_irap(input_file, true);
    test_assert_true(geo_surface_equal_header(surface, surirap));

    geo_surface_free(surirap);
    geo_surface_free(surface);
}

int main(int argc, char **argv) {
    char *input_file = argv[1];
    char *broken_file1 = argv[2];

    test_load(input_file, broken_file1);
    test_fprintf(input_file);
    test_create_new(input_file);
    exit(0);
}
