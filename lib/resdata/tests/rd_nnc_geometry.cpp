#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_nnc_geometry.hpp>

void test_create_empty() {
    rd_grid_type *grid = rd_grid_alloc_rectangular(10, 10, 10, 1, 1, 1, NULL);
    rd_nnc_geometry_type *nnc_geo = rd_nnc_geometry_alloc(grid);
    test_assert_true(rd_nnc_geometry_is_instance(nnc_geo));
    test_assert_int_equal(rd_nnc_geometry_size(nnc_geo), 0);
    rd_nnc_geometry_free(nnc_geo);
    rd_grid_free(grid);
}

void test_create_simple() {
    rd::util::TestArea ta("nnc_geometry");
    {
        int nx = 10;
        int ny = 10;
        int nz = 10;
        rd_grid_type *grid0 =
            rd_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1, NULL);

        rd_grid_add_self_nnc(grid0, 0, nx * ny + 0, 0);
        rd_grid_add_self_nnc(grid0, 1, nx * ny + 1, 1);
        rd_grid_add_self_nnc(grid0, 2, nx * ny + 2, 2);
        {
            rd_nnc_geometry_type *nnc_geo = rd_nnc_geometry_alloc(grid0);
            test_assert_int_equal(rd_nnc_geometry_size(nnc_geo), 3);

            /*
        Create a dummy INIT file which *ony* contains a TRANNC keyword with the correct size.
      */
            {
                rd_kw_type *trann_nnc = rd_kw_alloc(
                    TRANNNC_KW, rd_nnc_geometry_size(nnc_geo), RD_FLOAT);
                fortio_type *f =
                    fortio_open_writer("TEST.INIT", false, RD_ENDIAN_FLIP);

                for (int i = 0; i < rd_kw_get_size(trann_nnc); i++)
                    rd_kw_iset_float(trann_nnc, i, i * 1.0);

                rd_kw_fwrite(trann_nnc, f);
                fortio_fclose(f);
                rd_kw_free(trann_nnc);
            }
            rd_nnc_geometry_free(nnc_geo);
        }
        rd_grid_free(grid0);
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_create_empty();
    test_create_simple();
}
