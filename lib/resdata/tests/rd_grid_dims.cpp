#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_grid_dims.hpp>

void test_grid(const char *grid_filename, const char *data_filename) {
    rd_grid_type *rd_grid = rd_grid_alloc(grid_filename);
    rd_grid_dims_type *grid_dims =
        rd_grid_dims_alloc(grid_filename, data_filename);

    test_assert_not_NULL(grid_dims);
    test_assert_int_equal(rd_grid_get_num_lgr(rd_grid) + 1,
                          rd_grid_dims_get_num_grids(grid_dims));
    for (int i = 0; i < rd_grid_dims_get_num_grids(grid_dims); i++) {

        grid_dims_type d1 = rd_grid_iget_dims(rd_grid, i);
        const grid_dims_type *d2 = rd_grid_dims_iget_dims(grid_dims, i);

        test_assert_int_equal(d1.nx, d2->nx);
        test_assert_int_equal(d1.ny, d2->ny);
        test_assert_int_equal(d1.nz, d2->nz);

        if (data_filename)
            test_assert_int_equal(d1.nactive, d2->nactive);
    }
}

void test_dims() {
    grid_dims_type d1;
    grid_dims_type *d2 = grid_dims_alloc(100, 100, 100, 0);

    grid_dims_init(&d1, 100, 100, 100, 0);

    test_assert_int_equal(d1.nx, d2->nx);
    test_assert_int_equal(d1.ny, d2->ny);
    test_assert_int_equal(d1.nz, d2->nz);
    test_assert_int_equal(d1.nactive, d2->nactive);

    grid_dims_free(d2);
}

int main(int argc, char **argv) {
    signal(
        SIGSEGV,
        util_abort_signal); /* Segmentation violation, i.e. overwriting memory ... */

    if (argc == 1) {
        rd_grid_dims_type *grid_dims = rd_grid_dims_alloc(argv[0], NULL);
        test_assert_NULL(grid_dims);
        test_dims();
    } else {
        const char *GRID_file = argv[1];
        char *data_file;

        if (argc == 3)
            data_file = argv[2];
        else
            data_file = NULL;

        test_grid(GRID_file, data_file);
    }

    exit(0);
}
