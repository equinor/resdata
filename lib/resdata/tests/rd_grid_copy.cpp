#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>

void test_copy_grid(const rd_grid_type *grid) {
    rd_grid_type *grid_copy = rd_grid_alloc_copy(grid);
    test_assert_true(rd_grid_compare(grid, grid_copy, true, true, true));
    rd_grid_free(grid_copy);
}

void test_copy_grid_file(const char *filename) {
    rd_grid_type *src_grid = rd_grid_alloc(filename);
    rd_grid_type *copy_grid = rd_grid_alloc_copy(src_grid);
    test_assert_true(rd_grid_compare(src_grid, copy_grid, true, true, true));
    rd_grid_free(copy_grid);
    rd_grid_free(src_grid);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(10, 11, 12, 1, 2, 3, NULL);
        test_copy_grid(grid);
        rd_grid_free(grid);
    } else {
        test_copy_grid_file(argv[1]);
    }
    exit(0);
}
