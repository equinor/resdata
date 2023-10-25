#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>

void invalid_call1(void *arg) {
    rd_grid_type *grid = rd_grid_safe_cast(arg);
    double x, y, z;

    rd_grid_get_corner_xyz(grid, 10, 10, 11, &x, &y, &z);
}

void test_invalid(rd_grid_type *grid) {
    test_assert_util_abort("rd_grid_get_corner_xyz", invalid_call1, grid);
}

void test_OK(const rd_grid_type *grid) {
    double x, y, z;
    double x8[8];
    double y8[8];
    double z8[8];

    rd_grid_get_corner_xyz(grid, 0, 0, 0, &x, &y, &z);
    test_assert_double_equal(x, 0);
    test_assert_double_equal(y, 0);
    test_assert_double_equal(z, 0);

    rd_grid_get_corner_xyz(grid, 4, 5, 6, &x, &y, &z);
    test_assert_double_equal(x, 4);
    test_assert_double_equal(y, 5);
    test_assert_double_equal(z, 6);

    rd_grid_get_corner_xyz(grid, 10, 10, 10, &x, &y, &z);
    test_assert_double_equal(x, 10);
    test_assert_double_equal(y, 10);
    test_assert_double_equal(z, 10);

    rd_grid_export_cell_corners1(grid, 456, x8, y8, z8);
    for (int i = 0; i < 8; i++) {
        rd_grid_get_cell_corner_xyz1(grid, 456, i, &x, &y, &z);
        test_assert_double_equal(x, x8[i]);
        test_assert_double_equal(y, y8[i]);
        test_assert_double_equal(z, z8[i]);
    }
}

int main(int argc, char **argv) {
    rd_grid_type *grid = rd_grid_alloc_rectangular(10, 10, 10, 1, 1, 1, NULL);

    test_invalid(grid);
    test_OK(grid);

    rd_grid_free(grid);
    exit(0);
}
