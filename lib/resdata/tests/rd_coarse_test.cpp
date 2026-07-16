#include <cstdlib>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_coarse_cell.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <filesystem>

namespace fs = std::filesystem;

void test_coarse_cell(const rd_grid_type *grid, rd_coarse_cell_type *cell) {
    const int_vector_type *global_index_list =
        rd_coarse_cell_get_index_vector(cell);
    const int *ijk = rd_coarse_cell_get_box_ptr(cell);
    int prev_active = 0;

    for (int c = 0; c < rd_coarse_cell_get_size(cell); c++) {
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
    fs::path case_path(argv[1]);
    std::string egrid_file =
        rd::filename(case_path, RD_EGRID_FILE, false, 0).string();
    std::string rst_file =
        rd::filename(case_path, RD_RESTART_FILE, false, 0).string();
    std::string init_file =
        rd::filename(case_path, RD_INIT_FILE, false, 0).string();

    rd_grid_ptr GRID = read_grid(egrid_file);
    rd_file_ptr RST_file = rd::File::open(rst_file);
    rd_file_ptr INIT_file = rd::File::open(init_file);

    {
        test_assert_true(rd_grid_have_coarse_cells(GRID.get()));
        test_assert_int_equal(rd_grid_get_num_coarse_groups(GRID.get()), 3384);
    }

    {
        const rd_kw_type *swat0 = RST_file->get_kw("SWAT", 0);
        const rd_kw_type *porv = INIT_file->get_kw("PORV", 0);

        test_assert_int_equal(rd_kw_get_size(swat0),
                              rd_grid_get_active_size(GRID.get()));
        test_assert_int_equal(rd_kw_get_size(porv),
                              rd_grid_get_global_size(GRID.get()));
    }

    for (int ic = 0; ic < rd_grid_get_num_coarse_groups(GRID.get()); ic++) {
        rd_coarse_cell_type *coarse_cell =
            rd_grid_iget_coarse_group(GRID.get(), ic);
        test_coarse_cell(GRID.get(), coarse_cell);
    }
    exit(0);
}
