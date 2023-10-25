#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_coarse_cell.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>

void test_coarse_cell(const rd_grid_type *grid, rd_coarse_cell_type *cell) {
    const int_vector_type *global_index_list =
        rd_coarse_cell_get_index_vector(cell);
    const int *ijk = rd_coarse_cell_get_box_ptr(cell);
    int c;
    int prev_active = 0;

    for (c = 0; c < rd_coarse_cell_get_size(cell); c++) {
        int gi = int_vector_iget(global_index_list, c);
        int i, j, k;

        /* The coordinates are right */
        rd_grid_get_ijk1(grid, gi, &i, &j, &k);
        if ((i < ijk[0]) || (i > ijk[1]))
            test_error_exit("i:%d not inside range [%d,%d] \n", i, ijk[0],
                            ijk[1]);

        if ((j < ijk[2]) || (j > ijk[3]))
            test_error_exit("j:%d not inside range [%d,%d] \n", j, ijk[2],
                            ijk[3]);

        if ((k < ijk[4]) || (k > ijk[5]))
            test_error_exit("k:%d not inside range [%d,%d] \n", k, ijk[4],
                            ijk[4]);

        if (c == 0)
            prev_active = rd_grid_get_active_index1(grid, gi);
        else {
            /* All the cells have the same active value */
            int this_active = rd_grid_get_active_index1(grid, gi);
            test_assert_int_equal(prev_active, this_active);
            prev_active = this_active;
        }
    }
}

int main(int argc, char **argv) {
    const char *case_path = argv[1];
    char *egrid_file =
        rd_alloc_filename(NULL, case_path, RD_EGRID_FILE, false, 0);
    char *rst_file =
        rd_alloc_filename(NULL, case_path, RD_RESTART_FILE, false, 0);
    char *init_file =
        rd_alloc_filename(NULL, case_path, RD_INIT_FILE, false, 0);

    rd_grid_type *GRID = rd_grid_alloc(egrid_file);
    rd_file_type *RST_file = rd_file_open(rst_file, 0);
    rd_file_type *INIT_file = rd_file_open(init_file, 0);

    {
        test_assert_true(rd_grid_have_coarse_cells(GRID));
        test_assert_int_equal(rd_grid_get_num_coarse_groups(GRID), 3384);
    }

    {
        const rd_kw_type *swat0 = rd_file_iget_named_kw(RST_file, "SWAT", 0);
        const rd_kw_type *porv = rd_file_iget_named_kw(INIT_file, "PORV", 0);

        test_assert_int_equal(rd_kw_get_size(swat0),
                              rd_grid_get_active_size(GRID));
        test_assert_int_equal(rd_kw_get_size(porv),
                              rd_grid_get_global_size(GRID));
    }

    {
        int ic;
        for (ic = 0; ic < rd_grid_get_num_coarse_groups(GRID); ic++) {
            rd_coarse_cell_type *coarse_cell =
                rd_grid_iget_coarse_group(GRID, ic);
            test_coarse_cell(GRID, coarse_cell);
        }
    }

    rd_file_close(INIT_file);
    rd_file_close(RST_file);
    rd_grid_free(GRID);
    exit(0);
}
