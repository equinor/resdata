#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>

void test_copy_grid(const rd_grid_ptr &grid) {
    rd_grid_ptr grid_copy = copy_grid(grid.get());
    test_assert_true(
        rd_grid_compare(grid.get(), grid_copy.get(), true, true, true));
}

void test_copy_grid_file(const char *filename) {
    rd_grid_ptr src_grid = read_grid(filename);
    rd_grid_ptr grid_copy = copy_grid(src_grid.get());
    test_assert_true(
        rd_grid_compare(src_grid.get(), grid_copy.get(), true, true, true));
}

int main(int argc, char **argv) {
    if (argc < 2) {
        rd_grid_ptr grid = make_rectangular_grid(10, 11, 12, 1, 2, 3, NULL);
        test_copy_grid(grid);
    } else {
        test_copy_grid_file(argv[1]);
    }
    exit(0);
}
