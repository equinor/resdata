#include <cstdlib>

#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include <ert/util/test_util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_coarse_cell.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>

namespace fs = std::filesystem;

void test_coarse_cell(const rd_grid_type *grid, rd_coarse_cell_type *cell) {
    auto index_vector = rd_coarse_cell_get_index_vector(cell);
    int first_active = rd_grid_get_active_index1(grid, index_vector.at(0));
    for (int gi : index_vector) {
        /* All the cells have the same active value */
        int this_active = rd_grid_get_active_index1(grid, gi);
        test_assert_int_equal(first_active, this_active);
    }
}

int main(int argc, char **argv) {
    fs::path case_path(argv[1]);
    std::string egrid_file =
        rd::filename(case_path, FileType::EGRID, false, 0).string();
    std::string rst_file =
        rd::filename(case_path, FileType::RESTART, false, 0).string();
    std::string init_file =
        rd::filename(case_path, FileType::INIT, false, 0).string();

    rd_grid_ptr GRID = read_grid(egrid_file);
    std::unique_ptr<rd::File> RST_file = rd::File::open(rst_file);
    std::unique_ptr<rd::File> INIT_file = rd::File::open(init_file);

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
