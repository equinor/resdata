#include <vector>

#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>

void test_1() {

    rd::util::TestArea ta("test1");
    {
        const char *filename = "FILE.EGRID";

        rd_grid_type *grid_write =
            rd_grid_alloc_rectangular(2, 3, 1, 1, 1, 1, NULL);
        rd_grid_fwrite_EGRID(grid_write, filename, true);
        rd_grid_free(grid_write);

        rd_file_type *rd_file = rd_file_open(filename, 0);
        rd_kw_type *filehead_kw =
            rd_file_iget_named_kw(rd_file, FILEHEAD_KW, 0);
        int *filehead_data = rd_kw_get_int_ptr(filehead_kw);
        filehead_data[FILEHEAD_DUALP_INDEX] = FILEHEAD_DUAL_POROSITY;

        rd_kw_type *actnum_kw = rd_file_iget_named_kw(rd_file, ACTNUM_KW, 0);
        int *actnum_data = rd_kw_get_int_ptr(actnum_kw);
        actnum_data[0] = 1;
        actnum_data[1] = 2;
        actnum_data[2] = 2;
        actnum_data[3] = 0;
        actnum_data[4] = 1;
        actnum_data[5] = 1;
        const char *filename1 = "FILE1.EGRID";
        fortio_type *f = fortio_open_writer(filename1, false, RD_ENDIAN_FLIP);
        rd_file_fwrite_fortio(rd_file, f, 0);
        fortio_fclose(f);
        rd_file_close(rd_file);

        std::vector<int> ext_actnum = {0, 1, 0, 1, 1, 1};
        rd_grid_type *grid =
            rd_grid_alloc_ext_actnum(filename1, ext_actnum.data());
        test_assert_int_equal(4, rd_grid_get_nactive(grid));
        test_assert_int_equal(0, rd_grid_get_nactive_fracture(grid));
        test_assert_true(!rd_grid_cell_active1(grid, 0));

        test_assert_true(!rd_grid_cell_active1(grid, 2));
        test_assert_true(rd_grid_cell_active1(grid, 3));
        test_assert_true(rd_grid_cell_active1(grid, 4));
        test_assert_true(rd_grid_cell_active1(grid, 5));

        rd_grid_free(grid);
    }
}

int main() { test_1(); }
