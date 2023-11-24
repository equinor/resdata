#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>

#include <string>
#include <filesystem>
#include <iostream>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>

namespace fs = std::filesystem;

double err(double a, double b) { return (a - b) / a; }

void test_dxdydz(const std::string &grid_fname, const std::string &init_fname) {
    double eps_x = 1e-4;
    double eps_y = 1e-4;
    double eps_z = 1e-3;
    rd_grid_type *grid = rd_grid_alloc(grid_fname.c_str());
    if (grid == NULL) {
        std::cerr << "Could not open " << grid_fname << std::endl;
        exit(-1);
    }
    rd_file_type *init_file = rd_file_open(init_fname.c_str(), 0);
    if (init_file == NULL) {
        std::cerr << "Could not open " << init_fname << std::endl;
        exit(-1);
    }
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
    if (argc < 2) {
        std::cerr << "Must give case as input" << std::endl;
        exit(-1);
    }
    const std::string rd_case = argv[1];
    fs::path grid_file = rd_case + ".EGRID";
    fs::path init_file = rd_case + ".INIT";

    if (!fs::exists(grid_file) || !fs::exists(init_file)) {
        std::cerr << "Given case " << rd_case << " without grid or init"
                  << std::endl;
        exit(-1);
    }

    test_dxdydz(grid_file, init_file);

    exit(0);
}
