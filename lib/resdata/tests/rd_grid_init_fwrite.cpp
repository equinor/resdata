#include <ios>
#include <string>
#include <vector>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

static void rd_grid_fwrite_depth(rd_grid_type *grid, ERT::FortIO &init_file,
                                 UnitSystem output_unit) {
    auto depth_kw = make_rd_kw("DEPTH", rd_grid_get_nactive(grid), RD_FLOAT);
    {
        float *depth_ptr = (float *)rd_kw_get_ptr(depth_kw.get());
        for (int i = 0; i < rd_grid_get_nactive(grid); i++)
            depth_ptr[i] = rd_grid_get_cdepth1A(grid, i);
    }
    rd_kw_scale_float(depth_kw.get(),
                      rd_grid_output_scaling(grid, output_unit));
    rd_kw_fwrite(depth_kw.get(), init_file);
}

void test_write_depth(rd_grid_type *grid) {
    rd::util::TestArea ta("write_depth");
    {
        ERT::FortIO init_file("INIT", std::ios_base::out);
        rd_grid_fwrite_depth(grid, init_file, UnitSystem::METRIC);
    }
    {
        auto init_file = rd::File::open("INIT");
        rd_kw_type *depth = init_file->get_kw("DEPTH", 0);

        test_assert_int_equal(rd_kw_get_size(depth), rd_grid_get_nactive(grid));
        for (int i = 0; i < rd_grid_get_nactive(grid); i++)
            test_assert_double_equal(rd_kw_iget_as_double(depth, i),
                                     rd_grid_get_cdepth1A(grid, i));
    }
}

static void rd_grid_fwrite_dims(const rd_grid_type *grid,
                                ERT::FortIO &init_file,
                                UnitSystem output_unit) {
    auto dx = make_rd_kw("DX", rd_grid_get_nactive(grid), RD_FLOAT);
    auto dy = make_rd_kw("DY", rd_grid_get_nactive(grid), RD_FLOAT);
    auto dz = make_rd_kw("DZ", rd_grid_get_nactive(grid), RD_FLOAT);
    {
        {
            float *dx_ptr = (float *)rd_kw_get_ptr(dx.get());
            float *dy_ptr = (float *)rd_kw_get_ptr(dy.get());
            float *dz_ptr = (float *)rd_kw_get_ptr(dz.get());

            for (int i = 0; i < rd_grid_get_nactive(grid); i++) {
                dx_ptr[i] = rd_grid_get_cell_dx1A(grid, i);
                dy_ptr[i] = rd_grid_get_cell_dy1A(grid, i);
                dz_ptr[i] = rd_grid_get_cell_dz1A(grid, i);
            }
        }

        {
            float scale_factor = rd_grid_output_scaling(grid, output_unit);
            rd_kw_scale_float(dx.get(), scale_factor);
            rd_kw_scale_float(dy.get(), scale_factor);
            rd_kw_scale_float(dz.get(), scale_factor);
        }
    }
    rd_kw_fwrite(dx.get(), init_file);
    rd_kw_fwrite(dy.get(), init_file);
    rd_kw_fwrite(dz.get(), init_file);
}

void test_write_dims(const rd_grid_type *grid) {
    rd::util::TestArea ta("write_dims");
    {
        ERT::FortIO init_file("INIT", std::ios_base::out);
        rd_grid_fwrite_dims(grid, init_file, UnitSystem::METRIC);
    }
    {
        auto init_file = rd::File::open("INIT");
        rd_kw_type *DX = init_file->get_kw("DX", 0);
        rd_kw_type *DY = init_file->get_kw("DY", 0);
        rd_kw_type *DZ = init_file->get_kw("DZ", 0);

        test_assert_int_equal(rd_kw_get_size(DX), rd_grid_get_nactive(grid));
        test_assert_int_equal(rd_kw_get_size(DY), rd_grid_get_nactive(grid));
        test_assert_int_equal(rd_kw_get_size(DZ), rd_grid_get_nactive(grid));
        for (int i = 0; i < rd_grid_get_nactive(grid); i++) {
            test_assert_double_equal(rd_kw_iget_as_double(DX, i),
                                     rd_grid_get_cell_dx1A(grid, i));
            test_assert_double_equal(rd_kw_iget_as_double(DY, i),
                                     rd_grid_get_cell_dy1A(grid, i));
            test_assert_double_equal(rd_kw_iget_as_double(DZ, i),
                                     rd_grid_get_cell_dz1A(grid, i));
        }
    }
}

rd_grid_ptr create_grid() {
    int nx = 10;
    int ny = 10;
    int nz = 8;
    std::vector<int> actnum(nx * ny * nz, 1);

    return make_rectangular_grid(nx, ny, nz, 1, 1, 1, actnum.data());
}

int main(int argc, char **argv) {
    util_install_signals();
    {
        rd_grid_ptr grid = create_grid();

        test_write_depth(grid.get());
        test_write_dims(grid.get());
    }
}
