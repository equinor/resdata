#include <resdata/rd_nnc_data.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_nnc_geometry.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>

#include <ert/util/util.h>
#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

void test_alloc_global_only(bool data_in_file) {
    rd::util::TestArea ta("nnc-INIT");
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

                if (data_in_file) {
                    for (int i = 0; i < rd_kw_get_size(trann_nnc); i++)
                        rd_kw_iset_float(trann_nnc, i, i * 1.5);

                    rd_kw_fwrite(trann_nnc, f);
                }
                fortio_fclose(f);
                rd_kw_free(trann_nnc);
            }

            rd_file_type *init_file = rd_file_open("TEST.INIT", 0);
            rd_file_view_type *view_file = rd_file_get_global_view(init_file);

            rd_nnc_data_type *nnc_geo_data =
                rd_nnc_data_alloc_tran(grid0, nnc_geo, view_file);

            if (data_in_file) {

                int nnc_data_size = rd_nnc_data_get_size(nnc_geo_data);
                test_assert_true(rd_file_view_has_kw(view_file, TRANNNC_KW));
                test_assert_true(nnc_data_size == 3);
                const double *values = rd_nnc_data_get_values(nnc_geo_data);
                test_assert_double_equal(values[0], 0);
                test_assert_double_equal(values[1], 1.5);
                test_assert_double_equal(values[2], 3.0);
            } else
                test_assert_NULL(nnc_geo_data);

            if (data_in_file)
                rd_nnc_data_free(nnc_geo_data);
            rd_nnc_geometry_free(nnc_geo);
            rd_file_close(init_file);
        }
        rd_grid_free(grid0);
    }
}

int main(int argc, char **argv) {

    test_alloc_global_only(true);
    test_alloc_global_only(false);

    return 0;
}
