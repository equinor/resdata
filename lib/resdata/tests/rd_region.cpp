#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>

void test_list(int volume, int nactive, rd_region_type *region) {
    const int_vector_type *active_list;
    const int_vector_type *global_list;
    active_list = rd_region_get_active_list(region);
    global_list = rd_region_get_global_list(region);
    test_assert_int_equal(nactive, int_vector_size(active_list));
    test_assert_int_equal(volume, int_vector_size(global_list));

    rd_region_deselect_all(region);
    active_list = rd_region_get_active_list(region);
    global_list = rd_region_get_global_list(region);
    test_assert_int_equal(0, int_vector_size(active_list));
    test_assert_int_equal(0, int_vector_size(global_list));
}

void test_slice(const rd_grid_type *grid) {
    int nx = rd_grid_get_nx(grid);
    int ny = rd_grid_get_ny(grid);
    int nz = rd_grid_get_nz(grid);
    int nactive = rd_grid_get_nactive(grid);
    rd_region_type *region = rd_region_alloc(grid, false);

    rd_region_select_i1i2(region, 0, nx - 1);
    test_list(nx * ny * nz, nactive, region);
    rd_region_select_j1j2(region, 0, ny - 1);
    test_list(nx * ny * nz, nactive, region);
    rd_region_select_k1k2(region, 0, nz - 1);
    test_list(nx * ny * nz, nactive, region);

    rd_region_free(region);
}

int main(int argc, char **argv) {
    const char *grid_file = argv[1];
    rd_grid_type *grid = rd_grid_alloc(grid_file);

    test_slice(grid);

    rd_grid_free(grid);
    exit(0);
}
