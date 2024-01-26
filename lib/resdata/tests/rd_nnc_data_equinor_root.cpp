#include <resdata/rd_nnc_data.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_nnc_geometry.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

int find_index(rd_nnc_geometry_type *nnc_geo, int grid1, int grid2, int indx1,
               int indx2) {
    int index = -1;
    int nnc_size = rd_nnc_geometry_size(nnc_geo);
    for (int n = 0; n < nnc_size; n++) {
        const rd_nnc_pair_type *pair = rd_nnc_geometry_iget(nnc_geo, n);
        if (pair->grid_nr1 == grid1 && pair->grid_nr2 == grid2)
            if (pair->global_index1 == indx1 && pair->global_index2 == indx2) {
                index = n;
                break;
            }
    }
    return index;
}

void test_alloc_file_tran(char *filename) {
    char *grid_file_name =
        rd_alloc_filename(NULL, filename, RD_EGRID_FILE, false, -1);
    char *init_file_name =
        rd_alloc_filename(NULL, filename, RD_INIT_FILE, false, -1);
    rd_file_type *init_file = rd_file_open(init_file_name, 0);
    rd_grid_type *grid = rd_grid_alloc(grid_file_name);
    rd_nnc_geometry_type *nnc_geo = rd_nnc_geometry_alloc(grid);
    rd_file_view_type *view_file = rd_file_get_global_view(init_file);

    rd_nnc_data_type *nnc_geo_data =
        rd_nnc_data_alloc_tran(grid, nnc_geo, view_file);
    test_assert_true(rd_nnc_geometry_size(nnc_geo) ==
                     rd_nnc_data_get_size(nnc_geo_data));

    //These numerical values are hand-tuned the specific input file at:
    //${_eclpath}/Troll/MSW_LGR/2BRANCHES-CCEWELLPATH-NEW-SCH-TUNED-AR3
    int index;

    index = find_index(nnc_geo, 0, 0, 541, 14507);
    test_assert_double_equal(13.784438,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    index = find_index(nnc_geo, 0, 0, 48365, 118191);
    test_assert_double_equal(0.580284,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    index = find_index(nnc_geo, 0, 19, 42830, 211);
    test_assert_double_equal(0.571021,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    index = find_index(nnc_geo, 0, 79, 132406, 76);
    test_assert_double_equal(37.547710,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    index = find_index(nnc_geo, 18, 12, 303, 115);
    test_assert_double_equal(0.677443,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    index = find_index(nnc_geo, 72, 71, 255, 179);
    test_assert_double_equal(0.045813,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    index = find_index(nnc_geo, 110, 109, 271, 275);
    test_assert_double_equal(16.372242,
                             rd_nnc_data_iget_value(nnc_geo_data, index));

    rd_nnc_data_free(nnc_geo_data);
    rd_nnc_geometry_free(nnc_geo);
    rd_grid_free(grid);
    rd_file_close(init_file);
    free(grid_file_name);
    free(init_file_name);
}

void test_alloc_file_flux(char *filename, int file_num) {
    char *grid_file_name =
        rd_alloc_filename(NULL, filename, RD_EGRID_FILE, false, -1);
    char *restart_file_name =
        rd_alloc_filename(NULL, filename, RD_RESTART_FILE, false, file_num);

    rd_file_type *restart_file = rd_file_open(restart_file_name, 0);
    rd_grid_type *grid = rd_grid_alloc(grid_file_name);
    rd_nnc_geometry_type *nnc_geo = rd_nnc_geometry_alloc(grid);
    {
        rd_file_view_type *view_file = rd_file_get_global_view(restart_file);

        rd_nnc_data_type *nnc_flux_data =
            rd_nnc_data_alloc_wat_flux(grid, nnc_geo, view_file);
        test_assert_not_NULL(nnc_flux_data);
        rd_nnc_data_free(nnc_flux_data);
    }
    rd_nnc_geometry_free(nnc_geo);
    rd_grid_free(grid);
    rd_file_close(restart_file);
    free(grid_file_name);
    free(restart_file_name);
}

int main(int argc, char **argv) {
    test_alloc_file_tran(argv[1]);
    test_alloc_file_flux(argv[2], 0);
    test_alloc_file_flux(argv[2], 6);
    return 0;
}
