
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw.hpp>

/*
  This test is slightly awkward beacuse it tests quite internal implementation details.
*/

void test_transaction() {

    rd::util::TestArea ta("index_testing");
    {
        const char *file_name = "data_file";
        fortio_type *fortio =
            fortio_open_writer(file_name, false, RD_ENDIAN_FLIP);

        //creating the data file
        int data_size = 10;
        rd_kw_type *kw1 = rd_kw_alloc("TEST1_KW", data_size, RD_INT);
        for (int i = 0; i < data_size; ++i)
            rd_kw_iset_int(kw1, i, 537 + i);
        rd_kw_fwrite(kw1, fortio);

        data_size = 5;
        rd_kw_type *kw2 = rd_kw_alloc("TEST2_KW", data_size, RD_FLOAT);
        for (int i = 0; i < data_size; ++i)
            rd_kw_iset_float(kw2, i, 0.15 * i);
        rd_kw_fwrite(kw2, fortio);

        data_size = 3;
        rd_kw_type *kw3 = rd_kw_alloc("TEST3_KW", data_size, RD_FLOAT);
        for (int i = 0; i < data_size; i++)
            rd_kw_iset_float(kw3, i, 0.45 * i);
        rd_kw_fwrite(kw3, fortio);

        fortio_fclose(fortio);
        //finished creating data file

        rd_file_type *file = rd_file_open(file_name, 0);
        rd_file_view_type *file_view = rd_file_get_global_view(file);
        rd_file_kw_type *file_kw0 = rd_file_view_iget_file_kw(file_view, 0);
        rd_file_kw_type *file_kw1 = rd_file_view_iget_file_kw(file_view, 1);
        rd_file_kw_type *file_kw2 = rd_file_view_iget_file_kw(file_view, 2);

        rd_file_view_iget_kw(file_view, 0);
        test_assert_true(rd_file_kw_get_kw_ptr(file_kw0));
        test_assert_false(rd_file_kw_get_kw_ptr(file_kw1));
        test_assert_false(rd_file_kw_get_kw_ptr(file_kw2));
        rd_file_transaction_type *t1 =
            rd_file_view_start_transaction(file_view);

        rd_file_view_iget_kw(file_view, 0);
        rd_file_view_iget_kw(file_view, 1);
        rd_file_transaction_type *t2 =
            rd_file_view_start_transaction(file_view);

        rd_file_view_iget_kw(file_view, 0);
        rd_file_view_iget_kw(file_view, 1);
        rd_file_view_iget_kw(file_view, 1);
        rd_file_view_iget_kw(file_view, 2);

        rd_file_view_end_transaction(file_view, t2);

        test_assert_true(rd_file_kw_get_kw_ptr(file_kw0));
        test_assert_true(rd_file_kw_get_kw_ptr(file_kw1));
        test_assert_false(rd_file_kw_get_kw_ptr(file_kw2));
        rd_file_view_iget_kw(file_view, 2);

        rd_file_view_end_transaction(file_view, t1);
        test_assert_true(rd_file_kw_get_kw_ptr(file_kw0));
        test_assert_false(rd_file_kw_get_kw_ptr(file_kw1));
        test_assert_false(rd_file_kw_get_kw_ptr(file_kw2));

        rd_file_close(file);
        rd_kw_free(kw1);
        rd_kw_free(kw2);
        rd_kw_free(kw3);
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_transaction();
    exit(0);
}
