#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <ert/ecl/ecl_grid.hpp>

void test_layer(const ecl_grid_type *grid) {
    int g;

    for (g = 0; g < ecl_grid_get_global_size(grid); g += 25) {
        double x, y, z;
        int i, j, k;

        ecl_grid_get_xyz1(grid, g, &x, &y, &z);
        ecl_grid_get_ijk1(grid, g, &i, &j, &k);
        {
            int find_i, find_j;
            test_assert_true(
                ecl_grid_get_ij_from_xy(grid, x, y, k, &find_i, &find_j));
            test_assert_int_equal(i, find_i);
            test_assert_int_equal(j, find_j);
        }
    }
}

int main(int argc, char **argv) {
    ecl_grid_type *grid = ecl_grid_alloc(argv[1]);
    test_layer(grid);
    ecl_grid_free(grid);
}
