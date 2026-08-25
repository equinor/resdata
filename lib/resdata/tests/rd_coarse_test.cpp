#include <cstdlib>
#include <optional>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_coarse_cell.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <filesystem>

namespace fs = std::filesystem;

void test_coarse_cell(const rd_grid_type *grid, rd_coarse_cell_type *cell) {
    const auto &ijk = rd_coarse_cell_get_box(cell);
    std::optional<size_t> prev_active;

    bool first = true;
    for (size_t gi : rd_coarse_cell_get_index_vector(cell)) {
        size_t i, j, k;

        /* The coordinates are right */
        rd_grid_get_ijk1(grid, gi, &i, &j, &k);
        if ((i < ijk[0]) || (i > ijk[1]))
            test_error_exit("i:%zu not inside range [%zu,%zu] \n", i, ijk[0],
                            ijk[1]);

        if ((j < ijk[2]) || (j > ijk[3]))
            test_error_exit("j:%zu not inside range [%zu,%zu] \n", j, ijk[2],
                            ijk[3]);

        if ((k < ijk[4]) || (k > ijk[5]))
            test_error_exit("k:%zu not inside range [%zu,%zu] \n", k, ijk[4],
                            ijk[5]);

        if (first)
            prev_active = rd_grid_get_active_index1(grid, gi);
        else {
            /* All the cells have the same active value */
            auto this_active = rd_grid_get_active_index1(grid, gi);
            test_assert_true(prev_active == this_active);
            prev_active = this_active;
        }
        first = false;
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

    for (size_t ic = 0; ic < rd_grid_get_num_coarse_groups(GRID.get()); ic++) {
        rd_coarse_cell_type *coarse_cell =
            rd_grid_iget_coarse_group(GRID.get(), ic);
        test_coarse_cell(GRID.get(), coarse_cell);
    }
    exit(0);
}
