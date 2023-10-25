#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>

int main(int argc, char **argv) {
    const char *case_path = argv[1];
    char *grid_file =
        rd_alloc_filename(NULL, case_path, RD_EGRID_FILE, false, 0);
    char *init_file =
        rd_alloc_filename(NULL, case_path, RD_INIT_FILE, false, 0);
    char *rst_file =
        rd_alloc_filename(NULL, case_path, RD_RESTART_FILE, false, 0);

    rd_grid_type *rd_grid = rd_grid_alloc(grid_file);
    rd_file_type *RST_file = rd_file_open(rst_file, 0);
    rd_file_type *INIT_file = rd_file_open(init_file, 0);
    rd_file_type *GRID_file = rd_file_open(grid_file, 0);

    {
        rd_kw_type *actnum = rd_file_iget_named_kw(GRID_file, "ACTNUM", 0);
        rd_kw_type *swat = rd_file_iget_named_kw(RST_file, "SWAT", 0);
        rd_kw_type *permx = rd_file_iget_named_kw(INIT_file, "PERMX", 0);
        int fracture_size = rd_grid_get_nactive_fracture(rd_grid);
        int matrix_size = rd_grid_get_nactive(rd_grid);

        test_assert_int_equal(fracture_size + matrix_size,
                              rd_kw_get_size(swat));
        test_assert_int_equal(fracture_size + matrix_size,
                              rd_kw_get_size(permx));

        {
            int gi;
            int matrix_index = 0;
            int fracture_index = 0;

            for (gi = 0; gi < rd_grid_get_global_size(rd_grid); gi++) {
                if (rd_kw_iget_int(actnum, gi) & CELL_ACTIVE_MATRIX) {
                    test_assert_int_equal(
                        rd_grid_get_active_index1(rd_grid, gi), matrix_index);
                    test_assert_int_equal(
                        rd_grid_get_global_index1A(rd_grid, matrix_index), gi);
                    matrix_index++;
                }

                if (rd_kw_iget_int(actnum, gi) & CELL_ACTIVE_FRACTURE) {
                    test_assert_int_equal(
                        rd_grid_get_active_fracture_index1(rd_grid, gi),
                        fracture_index);
                    test_assert_int_equal(
                        rd_grid_get_global_index1F(rd_grid, fracture_index),
                        gi);
                    fracture_index++;
                }
            }
        }
    }

    rd_file_close(RST_file);
    rd_file_close(INIT_file);
    rd_grid_free(rd_grid);

    exit(0);
}
