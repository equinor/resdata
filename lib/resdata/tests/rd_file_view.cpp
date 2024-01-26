#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_file_kw.hpp>

void test_file_kw_equal() {
    rd_file_kw_type *kw1 = rd_file_kw_alloc0("PRESSURE", RD_FLOAT, 1000, 66);
    rd_file_kw_type *kw2 = rd_file_kw_alloc0("PRESSURE", RD_FLOAT, 1000, 66);
    rd_file_kw_type *kw3 = rd_file_kw_alloc0("SWAT", RD_FLOAT, 1000, 66);
    rd_file_kw_type *kw4 = rd_file_kw_alloc0("PRESSURE", RD_DOUBLE, 1000, 66);
    rd_file_kw_type *kw5 = rd_file_kw_alloc0("PRESSURE", RD_FLOAT, 10, 66);
    rd_file_kw_type *kw6 = rd_file_kw_alloc0("PRESSURE", RD_FLOAT, 1000, 67);

    test_assert_true(rd_file_kw_equal(kw1, kw1));
    test_assert_true(rd_file_kw_equal(kw1, kw2));
    test_assert_false(rd_file_kw_equal(kw1, kw3));
    test_assert_false(rd_file_kw_equal(kw1, kw4));
    test_assert_false(rd_file_kw_equal(kw1, kw5));
    test_assert_false(rd_file_kw_equal(kw1, kw6));

    rd_file_kw_free(kw6);
    rd_file_kw_free(kw5);
    rd_file_kw_free(kw4);
    rd_file_kw_free(kw3);
    rd_file_kw_free(kw2);
    rd_file_kw_free(kw1);
}

void test_create_file_kw() {
    rd_file_kw_type *file_kw0 =
        rd_file_kw_alloc0("PRESSURE", RD_FLOAT, 1000, 66);
    rd_file_kw_type *file_kw1 =
        rd_file_kw_alloc0("TEST1_KW", RD_FLOAT, 2000, 1066);
    rd_file_kw_type *file_kw2 =
        rd_file_kw_alloc0("TEST2_KW", RD_FLOAT, 3000, 2066);
    test_assert_string_equal(rd_file_kw_get_header(file_kw0), "PRESSURE");
    test_assert_int_equal(rd_file_kw_get_size(file_kw0), 1000);
    test_assert_true(
        rd_type_is_equal(rd_file_kw_get_data_type(file_kw0), RD_FLOAT));
    {
        rd::util::TestArea ta("file_kw");
        {
            FILE *ostream = util_fopen("file_kw", "w");
            rd_file_kw_fwrite(file_kw0, ostream);
            fclose(ostream);
        }
        {
            FILE *istream = util_fopen("file_kw", "r");
            rd_file_kw_type *disk_kw = rd_file_kw_fread_alloc(istream);
            test_assert_true(rd_file_kw_equal(file_kw0, disk_kw));

            /* Beyond the end of stream - return NULL */
            test_assert_NULL(rd_file_kw_fread_alloc(istream));
            rd_file_kw_free(disk_kw);
            fclose(istream);
        }

        {
            FILE *ostream = util_fopen("file_kw", "w");
            rd_file_kw_fwrite(file_kw0, ostream);
            rd_file_kw_fwrite(file_kw1, ostream);
            rd_file_kw_fwrite(file_kw2, ostream);
            fclose(ostream);
        }

        {
            FILE *istream = util_fopen("file_kw", "r");
            rd_file_kw_type **disk_kw =
                rd_file_kw_fread_alloc_multiple(istream, 3);
            test_assert_true(rd_file_kw_equal(file_kw0, disk_kw[0]));
            test_assert_true(rd_file_kw_equal(file_kw1, disk_kw[1]));
            test_assert_true(rd_file_kw_equal(file_kw2, disk_kw[2]));

            for (int i = 0; i < 3; i++)
                rd_file_kw_free(disk_kw[i]);
            free(disk_kw);
            fclose(istream);
        }
        {
            FILE *istream = util_fopen("file_kw", "r");
            test_assert_NULL(rd_file_kw_fread_alloc_multiple(istream, 10));
            fclose(istream);
        }
    }
    rd_file_kw_free(file_kw0);
    rd_file_kw_free(file_kw1);
    rd_file_kw_free(file_kw2);
}

int main(int argc, char **argv) {
    util_install_signals();
    test_file_kw_equal();
    test_create_file_kw();
}
