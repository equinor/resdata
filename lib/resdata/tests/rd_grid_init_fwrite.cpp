#include <cstdlib>

#include <ios>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <string>

static void rd_grid_fwrite_depth(rd_grid_type *grid, ERT::FortIO &init_file,
                                 ert_rd_unit_enum output_unit) {
    rd_kw_type *depth_kw =
        rd_kw_alloc("DEPTH", rd_grid_get_nactive(grid), RD_FLOAT);
    {
        float *depth_ptr = (float *)rd_kw_get_ptr(depth_kw);
        for (int i = 0; i < rd_grid_get_nactive(grid); i++)
            depth_ptr[i] = rd_grid_get_cdepth1A(grid, i);
    }
    rd_kw_scale_float(depth_kw, rd_grid_output_scaling(grid, output_unit));
    rd_kw_fwrite(depth_kw, init_file);
    rd_kw_free(depth_kw);
}

void test_write_depth(rd_grid_type *grid) {
    rd::util::TestArea ta("write_depth");
    {
        ERT::FortIO init_file("INIT", std::ios_base::out);
        rd_grid_fwrite_depth(grid, init_file, RD_METRIC_UNITS);
    }
    {
        auto init_file = open_rd_file(std::string("INIT"));
        rd_kw_type *depth = rd_file_iget_named_kw(init_file.get(), "DEPTH", 0);

        test_assert_int_equal(rd_kw_get_size(depth), rd_grid_get_nactive(grid));
        for (int i = 0; i < rd_grid_get_nactive(grid); i++)
            test_assert_double_equal(rd_kw_iget_as_double(depth, i),
                                     rd_grid_get_cdepth1A(grid, i));
    }
}

static void rd_grid_fwrite_dims(const rd_grid_type *grid,
                                ERT::FortIO &init_file,
                                ert_rd_unit_enum output_unit) {
    rd_kw_type *dx = rd_kw_alloc("DX", rd_grid_get_nactive(grid), RD_FLOAT);
    rd_kw_type *dy = rd_kw_alloc("DY", rd_grid_get_nactive(grid), RD_FLOAT);
    rd_kw_type *dz = rd_kw_alloc("DZ", rd_grid_get_nactive(grid), RD_FLOAT);
    {
        {
            float *dx_ptr = (float *)rd_kw_get_ptr(dx);
            float *dy_ptr = (float *)rd_kw_get_ptr(dy);
            float *dz_ptr = (float *)rd_kw_get_ptr(dz);

            for (int i = 0; i < rd_grid_get_nactive(grid); i++) {
                dx_ptr[i] = rd_grid_get_cell_dx1A(grid, i);
                dy_ptr[i] = rd_grid_get_cell_dy1A(grid, i);
                dz_ptr[i] = rd_grid_get_cell_dz1A(grid, i);
            }
        }

        {
            float scale_factor = rd_grid_output_scaling(grid, output_unit);
            rd_kw_scale_float(dx, scale_factor);
            rd_kw_scale_float(dy, scale_factor);
            rd_kw_scale_float(dz, scale_factor);
        }
    }
    rd_kw_fwrite(dx, init_file);
    rd_kw_fwrite(dy, init_file);
    rd_kw_fwrite(dz, init_file);
    rd_kw_free(dx);
    rd_kw_free(dy);
    rd_kw_free(dz);
}

void test_write_dims(const rd_grid_type *grid) {
    rd::util::TestArea ta("write_dims");
    {
        ERT::FortIO init_file("INIT", std::ios_base::out);
        rd_grid_fwrite_dims(grid, init_file, RD_METRIC_UNITS);
    }
    {
        auto init_file = open_rd_file(std::string("INIT"));
        rd_kw_type *DX = rd_file_iget_named_kw(init_file.get(), "DX", 0);
        rd_kw_type *DY = rd_file_iget_named_kw(init_file.get(), "DY", 0);
        rd_kw_type *DZ = rd_file_iget_named_kw(init_file.get(), "DZ", 0);

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
    auto actnum = make_int_vector(nx * ny * nz, 1);

    return make_rectangular_grid(nx, ny, nz, 1, 1, 1,
                                 int_vector_get_ptr(actnum.get()));
}

int main(int argc, char **argv) {
    util_install_signals();
    {
        rd_grid_ptr grid = create_grid();

        test_write_depth(grid.get());
        test_write_dims(grid.get());
    }
}
