#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_nnc_export.hpp>
#include <resdata/rd_kw_magic.hpp>

void test_get_tran(const char *name) {
    char *grid_file_name =
        rd_alloc_filename(NULL, name, RD_EGRID_FILE, false, -1);
    char *init_file_name =
        rd_alloc_filename(NULL, name, RD_INIT_FILE, false, -1);
    rd_grid_type *grid = rd_grid_alloc(grid_file_name);
    rd_file_type *grid_file = rd_file_open(grid_file_name, 0);
    rd_file_type *init_file = rd_file_open(init_file_name, 0);

    /* Get global */
    {
        rd_kw_type *tran_kw =
            rd_nnc_export_get_tran_kw(init_file, TRANNNC_KW, 0);
        test_assert_true(rd_kw_is_instance(tran_kw));
        test_assert_double_equal(0.85582769, rd_kw_iget_as_double(tran_kw, 0));
        test_assert_double_equal(0.24635284,
                                 rd_kw_iget_as_double(tran_kw, 7184));
    }
    test_assert_NULL(rd_nnc_export_get_tran_kw(init_file, TRANGL_KW, 0));
    test_assert_NULL(rd_nnc_export_get_tran_kw(init_file, TRANLL_KW, 0));
    test_assert_NULL(rd_nnc_export_get_tran_kw(init_file, "INVALID", 1));

    /* Get lgr_nr: 48 */
    {
        rd_kw_type *tran_kw =
            rd_nnc_export_get_tran_kw(init_file, TRANNNC_KW, 48);
        test_assert_true(rd_kw_is_instance(tran_kw));
        test_assert_int_equal(0, rd_kw_get_size(tran_kw));

        tran_kw = rd_nnc_export_get_tran_kw(init_file, TRANGL_KW, 48);
        test_assert_int_equal(282, rd_kw_get_size(tran_kw));
        test_assert_double_equal(22.922695, rd_kw_iget_as_double(tran_kw, 0));
        test_assert_double_equal(16.720325, rd_kw_iget_as_double(tran_kw, 281));
    }

    /* Get lgr_nr: 99 */
    {
        rd_kw_type *tran_kw =
            rd_nnc_export_get_tran_kw(init_file, TRANNNC_KW, 99);
        test_assert_true(rd_kw_is_instance(tran_kw));
        test_assert_int_equal(0, rd_kw_get_size(tran_kw));

        tran_kw = rd_nnc_export_get_tran_kw(init_file, TRANGL_KW, 99);
        test_assert_int_equal(693, rd_kw_get_size(tran_kw));
        test_assert_double_equal(0.25534782, rd_kw_iget_as_double(tran_kw, 0));
        test_assert_double_equal(0.12677453,
                                 rd_kw_iget_as_double(tran_kw, 692));
    }

    /* Get lgr_nr: 10 */
    {
        rd_kw_type *tran_kw =
            rd_nnc_export_get_tran_kw(init_file, TRANNNC_KW, 10);
        test_assert_true(rd_kw_is_instance(tran_kw));
        test_assert_int_equal(0, rd_kw_get_size(tran_kw));

        tran_kw = rd_nnc_export_get_tran_kw(init_file, TRANGL_KW, 10);
        test_assert_int_equal(260, rd_kw_get_size(tran_kw));
        test_assert_double_equal(0.87355447, rd_kw_iget_as_double(tran_kw, 0));
        test_assert_double_equal(26.921568, rd_kw_iget_as_double(tran_kw, 259));
    }

    /* Get lgr_nr: 110 */
    {
        rd_kw_type *tran_kw =
            rd_nnc_export_get_tran_kw(init_file, TRANNNC_KW, 110);
        test_assert_true(rd_kw_is_instance(tran_kw));
        test_assert_int_equal(0, rd_kw_get_size(tran_kw));

        tran_kw = rd_nnc_export_get_tran_kw(init_file, TRANGL_KW, 110);
        test_assert_int_equal(208, rd_kw_get_size(tran_kw));
        test_assert_double_equal(17.287283, rd_kw_iget_as_double(tran_kw, 0));
        test_assert_double_equal(569.26312, rd_kw_iget_as_double(tran_kw, 207));
    }

    free(init_file_name);
    free(grid_file_name);
    rd_grid_free(grid);
    rd_file_close(grid_file);
    rd_file_close(init_file);
}

void test_tranLL(const rd_grid_type *grid, const rd_file_type *init_file,
                 int lgr_nr1, int lgr_nr2, int size, double first,
                 double last) {

    rd_kw_type *rd_kw =
        rd_nnc_export_get_tranll_kw(grid, init_file, lgr_nr1, lgr_nr2);

    printf("lgr: %d -> %d \n", lgr_nr1, lgr_nr2);
    test_assert_not_NULL(rd_kw);
    test_assert_true(rd_kw_is_instance(rd_kw));
    test_assert_int_equal(size, rd_kw_get_size(rd_kw));
    test_assert_double_equal(first, rd_kw_iget_as_double(rd_kw, 0));
    test_assert_double_equal(last, rd_kw_iget_as_double(rd_kw, size - 1));
}

void test_get_tranLL(const char *name) {
    char *grid_file_name =
        rd_alloc_filename(NULL, name, RD_EGRID_FILE, false, -1);
    char *init_file_name =
        rd_alloc_filename(NULL, name, RD_INIT_FILE, false, -1);
    rd_grid_type *grid = rd_grid_alloc(grid_file_name);
    rd_file_type *grid_file = rd_file_open(grid_file_name, 0);
    rd_file_type *init_file = rd_file_open(init_file_name, 0);

    test_tranLL(grid, init_file, rd_grid_get_lgr_nr_from_name(grid, "LG003017"),
                rd_grid_get_lgr_nr_from_name(grid, "LG003018"), 172, 5.3957253,
                1.0099934);

    test_tranLL(grid, init_file, rd_grid_get_lgr_nr_from_name(grid, "LG002016"),
                rd_grid_get_lgr_nr_from_name(grid, "LG002017"), 93, 1.4638059,
                0.36407200);

    test_tranLL(grid, init_file, rd_grid_get_lgr_nr_from_name(grid, "LG002016"),
                rd_grid_get_lgr_nr_from_name(grid, "LG003016"), 56, 2.7360380,
                10.053267);

    test_tranLL(grid, init_file, rd_grid_get_lgr_nr_from_name(grid, "LG009027"),
                rd_grid_get_lgr_nr_from_name(grid, "LG009026"), 152, 155.47754,
                219.23553);

    test_tranLL(grid, init_file, rd_grid_get_lgr_nr_from_name(grid, "LG009027"),
                rd_grid_get_lgr_nr_from_name(grid, "LG008027"), 317,
                0.040260997, 0.0066288318);

    free(init_file_name);
    free(grid_file_name);
    rd_grid_free(grid);
    rd_file_close(grid_file);
    rd_file_close(init_file);
}

int main(int argc, char **argv) {
    test_get_tran(argv[1]);
    test_get_tranLL(argv[1]);
    exit(0);
}
