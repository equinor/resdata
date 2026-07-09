#include <cstdlib>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    fs::path case_path(argv[1]);
    std::string grid_file =
        rd::filename(case_path, RD_EGRID_FILE, false, 0).string();
    std::string init_file =
        rd::filename(case_path, RD_INIT_FILE, false, 0).string();
    std::string rst_file =
        rd::filename(case_path, RD_RESTART_FILE, false, 0).string();

    rd_grid_ptr rd_grid = read_grid(grid_file);
    rd_file_ptr RST_file = open_rd_file(rst_file);
    rd_file_ptr INIT_file = open_rd_file(init_file);
    rd_file_ptr GRID_file = open_rd_file(grid_file);

    {
        rd_kw_type *actnum =
            rd_file_iget_named_kw(GRID_file.get(), "ACTNUM", 0);
        rd_kw_type *swat = rd_file_iget_named_kw(RST_file.get(), "SWAT", 0);
        rd_kw_type *permx = rd_file_iget_named_kw(INIT_file.get(), "PERMX", 0);
        int fracture_size = rd_grid_get_nactive_fracture(rd_grid.get());
        int matrix_size = rd_grid_get_nactive(rd_grid.get());

        test_assert_int_equal(fracture_size + matrix_size,
                              rd_kw_get_size(swat));
        test_assert_int_equal(fracture_size + matrix_size,
                              rd_kw_get_size(permx));

        {
            int gi;
            int matrix_index = 0;
            int fracture_index = 0;

            for (gi = 0; gi < rd_grid_get_global_size(rd_grid.get()); gi++) {
                if (rd_kw_iget_int(actnum, gi) & CELL_ACTIVE_MATRIX) {
                    test_assert_int_equal(
                        rd_grid_get_active_index1(rd_grid.get(), gi),
                        matrix_index);
                    test_assert_int_equal(
                        rd_grid_get_global_index1A(rd_grid.get(), matrix_index),
                        gi);
                    matrix_index++;
                }

                if (rd_kw_iget_int(actnum, gi) & CELL_ACTIVE_FRACTURE) {
                    test_assert_int_equal(
                        rd_grid_get_active_fracture_index1(rd_grid.get(), gi),
                        fracture_index);
                    test_assert_int_equal(rd_grid_get_global_index1F(
                                              rd_grid.get(), fracture_index),
                                          gi);
                    fracture_index++;
                }
            }
        }
    }
    exit(0);
}
