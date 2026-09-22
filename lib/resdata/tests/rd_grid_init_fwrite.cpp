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
    rd::KW depth_kw{"DEPTH", rd_grid_get_nactive(grid), RD_FLOAT};
    {
        std::vector<float> &depth_ptr = depth_kw.get_vector<float>();
        for (int i = 0; i < rd_grid_get_nactive(grid); i++)
            depth_ptr[i] = rd_grid_get_cdepth1A(grid, i);
    }
    depth_kw.scale<float>(rd_grid_output_scaling(grid, output_unit));
    depth_kw.fwrite(init_file);
}

void test_write_depth(rd_grid_type *grid) {
    rd::util::TestArea ta("write_depth");
    {
        ERT::FortIO init_file("INIT", std::ios_base::out);
        rd_grid_fwrite_depth(grid, init_file, UnitSystem::METRIC);
    }
    {
        auto init_file = rd::File::open("INIT");
        rd::KW *depth = init_file->get_kw("DEPTH", 0);

        test_assert_int_equal(rd::kw_get_size(depth),
                              rd_grid_get_nactive(grid));
        for (int i = 0; i < rd_grid_get_nactive(grid); i++)
            test_assert_double_equal(depth->as_double(i),
                                     rd_grid_get_cdepth1A(grid, i));
    }
}

static void rd_grid_fwrite_dims(const rd_grid_type *grid,
                                ERT::FortIO &init_file,
                                UnitSystem output_unit) {
    rd::KW dx{"DX", rd_grid_get_nactive(grid), RD_FLOAT};
    rd::KW dy{"DY", rd_grid_get_nactive(grid), RD_FLOAT};
    rd::KW dz{"DZ", rd_grid_get_nactive(grid), RD_FLOAT};
    {
        {
            std::vector<float> &dx_ptr = dx.get_vector<float>();
            std::vector<float> &dy_ptr = dy.get_vector<float>();
            std::vector<float> &dz_ptr = dz.get_vector<float>();

            for (int i = 0; i < rd_grid_get_nactive(grid); i++) {
                dx_ptr[i] = rd_grid_get_cell_dx1A(grid, i);
                dy_ptr[i] = rd_grid_get_cell_dy1A(grid, i);
                dz_ptr[i] = rd_grid_get_cell_dz1A(grid, i);
            }
        }

        {
            float scale_factor = rd_grid_output_scaling(grid, output_unit);
            dx.scale<float>(scale_factor);
            dy.scale<float>(scale_factor);
            dz.scale<float>(scale_factor);
        }
    }
    dx.fwrite(init_file);
    dy.fwrite(init_file);
    dz.fwrite(init_file);
}

void test_write_dims(const rd_grid_type *grid) {
    rd::util::TestArea ta("write_dims");
    {
        ERT::FortIO init_file("INIT", std::ios_base::out);
        rd_grid_fwrite_dims(grid, init_file, UnitSystem::METRIC);
    }
    {
        auto init_file = rd::File::open("INIT");
        rd::KW *DX = init_file->get_kw("DX", 0);
        rd::KW *DY = init_file->get_kw("DY", 0);
        rd::KW *DZ = init_file->get_kw("DZ", 0);

        test_assert_int_equal(rd::kw_get_size(DX), rd_grid_get_nactive(grid));
        test_assert_int_equal(rd::kw_get_size(DY), rd_grid_get_nactive(grid));
        test_assert_int_equal(rd::kw_get_size(DZ), rd_grid_get_nactive(grid));
        for (int i = 0; i < rd_grid_get_nactive(grid); i++) {
            test_assert_double_equal(DX->as_double(i),
                                     rd_grid_get_cell_dx1A(grid, i));
            test_assert_double_equal(DY->as_double(i),
                                     rd_grid_get_cell_dy1A(grid, i));
            test_assert_double_equal(DZ->as_double(i),
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
