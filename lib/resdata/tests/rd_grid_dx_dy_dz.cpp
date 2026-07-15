#include <cstdlib>
#include <cmath>

#include <memory>
#include <string>
#include <filesystem>
#include <iostream>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>

namespace fs = std::filesystem;

double err(double a, double b) { return (a - b) / a; }

void test_dxdydz(const std::string &grid_fname, const std::string &init_fname) {
    double eps_x = 1e-4;
    double eps_y = 1e-4;
    double eps_z = 1e-3;
    rd_grid_ptr grid = read_grid(grid_fname);
    if (!grid) {
        std::cerr << "Could not open " << grid_fname << std::endl;
        exit(-1);
    }
    std::unique_ptr<rd::File> init_file;
    try {
        init_file = rd::File::open(init_fname);
    } catch (const std::ios_base::failure &e) {
        std::cerr << "Could not open " << init_fname << ": " << e.what()
                  << std::endl;
        exit(-1);
    }
    rd_kw_type *dx = rd_file_iget_named_kw(init_file.get(), "DX", 0);
    rd_kw_type *dy = rd_file_iget_named_kw(init_file.get(), "DY", 0);
    rd_kw_type *dz = rd_file_iget_named_kw(init_file.get(), "DZ", 0);
    for (int a = 0; a < rd_grid_get_active_size(grid.get()); a += 100) {
        int g = rd_grid_get_global_index1A(grid.get(), a);

        double dxg = rd_grid_get_cell_dx1(grid.get(), g);
        double dyg = rd_grid_get_cell_dy1(grid.get(), g);
        double dzg = rd_grid_get_cell_dz1(grid.get(), g);

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
