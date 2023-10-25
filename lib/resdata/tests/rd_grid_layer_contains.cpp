#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>

void test_layer(const rd_grid_type *grid) {
    int g;

    for (g = 0; g < rd_grid_get_global_size(grid); g += 25) {
        double x, y, z;
        int i, j, k;

        rd_grid_get_xyz1(grid, g, &x, &y, &z);
        rd_grid_get_ijk1(grid, g, &i, &j, &k);
        {
            int find_i, find_j;
            test_assert_true(
                rd_grid_get_ij_from_xy(grid, x, y, k, &find_i, &find_j));
            test_assert_int_equal(i, find_i);
            test_assert_int_equal(j, find_j);
        }
    }
}

int main(int argc, char **argv) {
    rd_grid_type *grid = rd_grid_alloc(argv[1]);
    test_layer(grid);
    rd_grid_free(grid);
}
