#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
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
    rd_grid_ptr grid0 = make_rectangular_grid(10, 10, 10, 1, 1, 1, NULL);
    rd_grid_add_self_nnc(grid0.get(), 5, 6, 0);
    rd_grid_add_self_nnc(grid0.get(), 5, 7, 1);
    rd_grid_add_self_nnc(grid0.get(), 8, 9, 2);

    verify_simple_nnc(grid0.get());
    {
        rd::util::TestArea ta("simple_nnc");
        rd_grid_fwrite_EGRID2(grid0.get(), "TEST.EGRID", RD_METRIC_UNITS);
        rd_grid_ptr grid1 = read_grid("TEST.EGRID");

        verify_simple_nnc(grid1.get());
    }
}

void overwrite_test() {
    rd_grid_ptr grid0 = make_rectangular_grid(10, 10, 10, 1, 1, 1, NULL);

    /*
     This first list of nnc will be overwritten, and will not survive
     the serialization to disk.
  */

    rd_grid_add_self_nnc(grid0.get(), 1, 2, 0);
    rd_grid_add_self_nnc(grid0.get(), 1, 3, 1);
    rd_grid_add_self_nnc(grid0.get(), 1, 4, 2);

    rd_grid_add_self_nnc(grid0.get(), 5, 6, 0);
    rd_grid_add_self_nnc(grid0.get(), 5, 7, 1);
    rd_grid_add_self_nnc(grid0.get(), 8, 9, 2);

    verify_simple_nnc(grid0.get());
    {
        rd::util::TestArea ta("overwrite");
        rd_grid_fwrite_EGRID2(grid0.get(), "TEST.EGRID", RD_METRIC_UNITS);
        rd_grid_ptr grid1 = read_grid("TEST.EGRID");

        verify_simple_nnc(grid1.get());
    }
}

void list_test() {
    rd_grid_ptr grid0 = make_rectangular_grid(10, 10, 10, 1, 1, 1, NULL);
    auto g1 = make_int_vector(0, 0);
    auto g2 = make_int_vector(0, 0);

    int_vector_append(g1.get(), 5);
    int_vector_append(g2.get(), 6);
    int_vector_append(g1.get(), 5);
    int_vector_append(g2.get(), 7);
    int_vector_append(g1.get(), 8);
    int_vector_append(g2.get(), 9);

    for (int i = 0; i < int_vector_size(g1.get()); i++)
        rd_grid_add_self_nnc(grid0.get(), int_vector_iget(g1.get(), i),
                             int_vector_iget(g2.get(), i), i);

    verify_simple_nnc(grid0.get());
    {
        rd::util::TestArea ta("list_test");
        rd_grid_fwrite_EGRID2(grid0.get(), "TEST.EGRID", RD_METRIC_UNITS);
        rd_grid_ptr grid1 = read_grid("TEST.EGRID");

        verify_simple_nnc(grid1.get());
    }
}

int main(int argc, char **argv) {
    simple_test();
    list_test();
    overwrite_test();
    exit(0);
}
