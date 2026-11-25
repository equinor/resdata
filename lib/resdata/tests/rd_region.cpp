#include <cstdlib>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_region.hpp>

void test_list(size_t volume, size_t nactive, rd_region_type *region) {
    {
        auto &active_list = rd_region_get_active_list(region);
        auto &global_list = rd_region_get_global_list(region);
        test_assert_size_t_equal(nactive, active_list.size());
        test_assert_size_t_equal(volume, global_list.size());
    }

    {
        rd_region_deselect_all(region);
        auto &active_list = rd_region_get_active_list(region);
        auto &global_list = rd_region_get_global_list(region);
        test_assert_size_t_equal(0, active_list.size());
        test_assert_size_t_equal(0, global_list.size());
    }
}

void test_slice(rd_grid_type *grid) {
    // The region selection API takes int index ranges
    int nx = static_cast<int>(rd_grid_get_nx(grid));
    int ny = static_cast<int>(rd_grid_get_ny(grid));
    int nz = static_cast<int>(rd_grid_get_nz(grid));
    size_t nactive = rd_grid_get_nactive(grid);
    size_t global_size = rd_grid_get_global_size(grid);
    rd_region_type *region = rd_region_alloc(grid, false);

    rd_region_select_i1i2(region, 0, nx - 1);
    test_list(global_size, nactive, region);
    rd_region_select_j1j2(region, 0, ny - 1);
    test_list(global_size, nactive, region);
    rd_region_select_k1k2(region, 0, nz - 1);
    test_list(global_size, nactive, region);

    rd_region_free(region);
}

int main(int argc, char **argv) {
    rd_grid_ptr grid = read_grid(argv[1]);
    test_slice(grid.get());
    exit(0);
}
