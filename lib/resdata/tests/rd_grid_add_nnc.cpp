#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_grid.hpp>

void verify_simple_nnc(const rd_grid_type *grid) {
    test_assert_not_NULL(rd_grid_get_cell_nnc_info1(grid, 5));
    test_assert_not_NULL(rd_grid_get_cell_nnc_info1(grid, 8));

    // Cell 5:
    {
        const nnc_info_type *nnc_info = rd_grid_get_cell_nnc_info1(grid, 5);
        const nnc_vector_type *nnc_vector = nnc_info_iget_vector(nnc_info, 0);
        test_assert_int_equal(2, nnc_vector_get_size(nnc_vector));
        test_assert_int_equal(6, nnc_vector_iget_grid_index(nnc_vector, 0));
        test_assert_int_equal(7, nnc_vector_iget_grid_index(nnc_vector, 1));

        test_assert_int_equal(0, nnc_vector_iget_nnc_index(nnc_vector, 0));
        test_assert_int_equal(1, nnc_vector_iget_nnc_index(nnc_vector, 1));
    }

    // Cell 8:
    {
        const nnc_info_type *nnc_info = rd_grid_get_cell_nnc_info1(grid, 8);
        const nnc_vector_type *nnc_vector = nnc_info_iget_vector(nnc_info, 0);
        test_assert_int_equal(1, nnc_vector_get_size(nnc_vector));
        test_assert_int_equal(9, nnc_vector_iget_grid_index(nnc_vector, 0));
        test_assert_int_equal(2, nnc_vector_iget_nnc_index(nnc_vector, 0));
    }
}

void simple_test() {
    rd_grid_type *grid0 = rd_grid_alloc_rectangular(10, 10, 10, 1, 1, 1, NULL);
    rd_grid_add_self_nnc(grid0, 5, 6, 0);
    rd_grid_add_self_nnc(grid0, 5, 7, 1);
    rd_grid_add_self_nnc(grid0, 8, 9, 2);

    verify_simple_nnc(grid0);
    {
        rd::util::TestArea ta("simple_nnc");
        rd_grid_type *grid1;
        rd_grid_fwrite_EGRID2(grid0, "TEST.EGRID", RD_METRIC_UNITS);
        grid1 = rd_grid_alloc("TEST.EGRID");

        verify_simple_nnc(grid1);
        rd_grid_free(grid1);
    }
    rd_grid_free(grid0);
}

void overwrite_test() {
    rd_grid_type *grid0 = rd_grid_alloc_rectangular(10, 10, 10, 1, 1, 1, NULL);

    /*
     This first list of nnc will be overwritten, and will not survive
     the serialization to disk.
  */

    rd_grid_add_self_nnc(grid0, 1, 2, 0);
    rd_grid_add_self_nnc(grid0, 1, 3, 1);
    rd_grid_add_self_nnc(grid0, 1, 4, 2);

    rd_grid_add_self_nnc(grid0, 5, 6, 0);
    rd_grid_add_self_nnc(grid0, 5, 7, 1);
    rd_grid_add_self_nnc(grid0, 8, 9, 2);

    verify_simple_nnc(grid0);
    {
        rd::util::TestArea ta("overwrite");
        rd_grid_type *grid1;
        rd_grid_fwrite_EGRID2(grid0, "TEST.EGRID", RD_METRIC_UNITS);
        grid1 = rd_grid_alloc("TEST.EGRID");

        verify_simple_nnc(grid1);
        rd_grid_free(grid1);
    }
    rd_grid_free(grid0);
}

void list_test() {
    rd_grid_type *grid0 = rd_grid_alloc_rectangular(10, 10, 10, 1, 1, 1, NULL);
    int_vector_type *g1 = int_vector_alloc(0, 0);
    int_vector_type *g2 = int_vector_alloc(0, 0);

    int_vector_append(g1, 5);
    int_vector_append(g2, 6);
    int_vector_append(g1, 5);
    int_vector_append(g2, 7);
    int_vector_append(g1, 8);
    int_vector_append(g2, 9);

    rd_grid_add_self_nnc_list(grid0, int_vector_get_ptr(g1),
                              int_vector_get_ptr(g2), int_vector_size(g1));

    verify_simple_nnc(grid0);
    {
        rd::util::TestArea ta("list_test");
        rd_grid_type *grid1;
        rd_grid_fwrite_EGRID2(grid0, "TEST.EGRID", RD_METRIC_UNITS);
        grid1 = rd_grid_alloc("TEST.EGRID");

        verify_simple_nnc(grid1);
        rd_grid_free(grid1);
    }
    int_vector_free(g1);
    int_vector_free(g2);
    rd_grid_free(grid0);
}

int main(int argc, char **argv) {
    simple_test();
    list_test();
    overwrite_test();
    exit(0);
}
