#include <cstdlib>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>

void test_file_kw_equal() {
    rd_file_kw_type kw1 = rd_file_kw_type(66, RD_FLOAT, 1000, "PRESSURE");
    rd_file_kw_type kw2 = rd_file_kw_type(66, RD_FLOAT, 1000, "PRESSURE");
    rd_file_kw_type kw3 = rd_file_kw_type(66, RD_FLOAT, 1000, "SWAT");
    rd_file_kw_type kw4 = rd_file_kw_type(66, RD_DOUBLE, 1000, "PRESSURE");
    rd_file_kw_type kw5 = rd_file_kw_type(66, RD_FLOAT, 10, "PRESSURE");
    rd_file_kw_type kw6 = rd_file_kw_type(67, RD_FLOAT, 1000, "PRESSURE");

    test_assert_true(kw1 == kw1);
    test_assert_true(kw1 == kw2);
    test_assert_false(kw1 == kw3);
    test_assert_false(kw1 == kw4);
    test_assert_false(kw1 == kw5);
    test_assert_false(kw1 == kw6);
}

void test_create_file_kw() {
    rd_file_kw_type file_kw0 = rd_file_kw_type(66, RD_FLOAT, 1000, "PRESSURE");
    rd_file_kw_type file_kw1 =
        rd_file_kw_type(1066, RD_FLOAT, 2000, "TEST1_KW");
    rd_file_kw_type file_kw2 =
        rd_file_kw_type(2066, RD_FLOAT, 3000, "TEST2_KW");
    test_assert_string_equal(rd_file_kw_get_header(&file_kw0), "PRESSURE");
    test_assert_int_equal(rd_file_kw_get_size(&file_kw0), 1000);
    test_assert_true(
        rd_type_is_equal(rd_file_kw_get_data_type(&file_kw0), RD_FLOAT));
    {
        rd::util::TestArea ta("file_kw");
        {
            FILE *ostream = util_fopen("file_kw", "w");
            rd_file_kw_fwrite(&file_kw0, ostream);
            fclose(ostream);
        }
        {
            FILE *istream = util_fopen("file_kw", "r");
            auto kws = rd_file_kw_fread(istream, 1);
            rd_file_kw_type *disk_kw = kws.at(0).get();
            test_assert_true(file_kw0 == *disk_kw);

            /* Beyond the end of stream - return NULL */
            test_assert_throw(rd_file_kw_fread(istream, 1), std::exception);
            fclose(istream);
        }

        {
            FILE *ostream = util_fopen("file_kw", "w");
            rd_file_kw_fwrite(&file_kw0, ostream);
            rd_file_kw_fwrite(&file_kw1, ostream);
            rd_file_kw_fwrite(&file_kw2, ostream);
            fclose(ostream);
        }

        {
            FILE *istream = util_fopen("file_kw", "r");
            auto disk_kw = rd_file_kw_fread(istream, 3);
            test_assert_true(file_kw0 == *disk_kw[0]);
            test_assert_true(file_kw1 == *disk_kw[1]);
            test_assert_true(file_kw2 == *disk_kw[2]);

            fclose(istream);
        }
        {
            FILE *istream = util_fopen("file_kw", "r");
            test_assert_throw(rd_file_kw_fread(istream, 10), std::exception);
            fclose(istream);
        }
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_file_kw_equal();
    test_create_file_kw();
}
