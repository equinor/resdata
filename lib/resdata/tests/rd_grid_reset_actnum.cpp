#include <cstdlib>

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
    rd_grid_ptr grid = make_rectangular_grid(nx, ny, nz, 1, 1, 1, actnum1);

    test_assert_int_equal(g, rd_grid_get_nactive(grid.get()));
    rd_grid_reset_actnum(grid.get(), actnum2);
    test_assert_int_equal(nactive, rd_grid_get_nactive(grid.get()));

    test_assert_int_equal(-1, rd_grid_get_active_index1(grid.get(), 0));
    test_assert_int_equal(0, rd_grid_get_active_index1(grid.get(), 1));
    test_assert_int_equal(-1, rd_grid_get_active_index1(grid.get(), 2));
    test_assert_int_equal(1, rd_grid_get_global_index1A(grid.get(), 0));
    test_assert_int_equal(3, rd_grid_get_global_index1A(grid.get(), 1));
    test_assert_int_equal(5, rd_grid_get_global_index1A(grid.get(), 2));

    rd_grid_reset_actnum(grid.get(), NULL);
    test_assert_int_equal(g, rd_grid_get_nactive(grid.get()));
    test_assert_int_equal(0, rd_grid_get_active_index1(grid.get(), 0));
    test_assert_int_equal(1, rd_grid_get_active_index1(grid.get(), 1));
    test_assert_int_equal(2, rd_grid_get_active_index1(grid.get(), 2));
    test_assert_int_equal(0, rd_grid_get_global_index1A(grid.get(), 0));
    test_assert_int_equal(1, rd_grid_get_global_index1A(grid.get(), 1));
    test_assert_int_equal(2, rd_grid_get_global_index1A(grid.get(), 2));

    exit(0);
}
