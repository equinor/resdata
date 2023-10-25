#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>

int main(int argc, char **argv) {
    const int nx = 5;
    const int ny = 4;
    const int nz = 2;
    const int g = nx * ny * nz;
    const int nactive = g - 9;
    const int actnum1[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    const int actnum2[] = {0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
                           1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
                           1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0};
    rd_grid_type *grid =
        rd_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1, actnum1);

    test_assert_int_equal(g, rd_grid_get_nactive(grid));
    rd_grid_reset_actnum(grid, actnum2);
    test_assert_int_equal(nactive, rd_grid_get_nactive(grid));

    test_assert_int_equal(-1, rd_grid_get_active_index1(grid, 0));
    test_assert_int_equal(0, rd_grid_get_active_index1(grid, 1));
    test_assert_int_equal(-1, rd_grid_get_active_index1(grid, 2));
    test_assert_int_equal(1, rd_grid_get_global_index1A(grid, 0));
    test_assert_int_equal(3, rd_grid_get_global_index1A(grid, 1));
    test_assert_int_equal(5, rd_grid_get_global_index1A(grid, 2));

    rd_grid_reset_actnum(grid, NULL);
    test_assert_int_equal(g, rd_grid_get_nactive(grid));
    test_assert_int_equal(0, rd_grid_get_active_index1(grid, 0));
    test_assert_int_equal(1, rd_grid_get_active_index1(grid, 1));
    test_assert_int_equal(2, rd_grid_get_active_index1(grid, 2));
    test_assert_int_equal(0, rd_grid_get_global_index1A(grid, 0));
    test_assert_int_equal(1, rd_grid_get_global_index1A(grid, 1));
    test_assert_int_equal(2, rd_grid_get_global_index1A(grid, 2));

    rd_grid_free(grid);
    exit(0);
}
