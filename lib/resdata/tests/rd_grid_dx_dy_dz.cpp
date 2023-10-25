#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>

#include <string>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>

double err(double a, double b) { return (a - b) / a; }

void test_dxdydz(const std::string &grid_fname, const std::string &init_fname) {
    double eps_x = 1e-4;
    double eps_y = 1e-4;
    double eps_z = 1e-3;
    rd_grid_type *grid = rd_grid_alloc(grid_fname.c_str());
    rd_file_type *init_file = rd_file_open(init_fname.c_str(), 0);
    rd_kw_type *dx = rd_file_iget_named_kw(init_file, "DX", 0);
    rd_kw_type *dy = rd_file_iget_named_kw(init_file, "DY", 0);
    rd_kw_type *dz = rd_file_iget_named_kw(init_file, "DZ", 0);
    for (int a = 0; a < rd_grid_get_active_size(grid); a += 100) {
        int g = rd_grid_get_global_index1A(grid, a);

        double dxg = rd_grid_get_cell_dx1(grid, g);
        double dyg = rd_grid_get_cell_dy1(grid, g);
        double dzg = rd_grid_get_cell_dz1(grid, g);

        double dxi = rd_kw_iget_float(dx, a);
        double dyi = rd_kw_iget_float(dy, a);
        double dzi = rd_kw_iget_float(dz, a);

        double err_x = fabs(err(dxg, dxi));
        double err_y = fabs(err(dyg, dyi));
        double err_z = fabs(err(dzg, dzi));

        test_assert_true(err_x < eps_x);
        test_assert_true(err_y < eps_y);
        test_assert_true(err_z < eps_z);
    }
    rd_file_close(init_file);
    rd_grid_free(grid);
}

int main(int argc, char **argv) {
    const std::string rd_case = argv[1];
    std::string grid_file = rd_case + ".EGRID";
    std::string init_file = rd_case + ".INIT";

    test_dxdydz(grid_file, init_file);

    exit(0);
}
