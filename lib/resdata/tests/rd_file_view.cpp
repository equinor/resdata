#include <cstdio>
#include <cstdlib>
#include <exception>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_type.hpp>
#include <resdata/rd_file_kw.hpp>

void test_file_kw_equal() {
    FileKW kw1(66, RD_FLOAT, 1000, "PRESSURE");
    FileKW kw2(66, RD_FLOAT, 1000, "PRESSURE");
    FileKW kw3(66, RD_FLOAT, 1000, "SWAT");
    FileKW kw4(66, RD_DOUBLE, 1000, "PRESSURE");
    FileKW kw5(66, RD_FLOAT, 10, "PRESSURE");
    FileKW kw6(67, RD_FLOAT, 1000, "PRESSURE");

    test_assert_true(kw1 == kw1);
    test_assert_true(kw1 == kw2);
    test_assert_false(kw1 == kw3);
    test_assert_false(kw1 == kw4);
    test_assert_false(kw1 == kw5);
    test_assert_false(kw1 == kw6);
}

void test_create_file_kw() {
    FileKW file_kw0(66, RD_FLOAT, 1000, "PRESSURE");
    FileKW file_kw1(1066, RD_FLOAT, 2000, "TEST1_KW");
    FileKW file_kw2(2066, RD_FLOAT, 3000, "TEST2_KW");
    test_assert_string_equal(file_kw0.get_header().c_str(), "PRESSURE");
    test_assert_int_equal(file_kw0.get_size(), 1000);
    test_assert_true(rd_type_is_equal(file_kw0.get_data_type(), RD_FLOAT));
    {
        rd::util::TestArea ta("file_kw");
        {
            FILE *ostream = util_fopen("file_kw", "w");
            file_kw0.write_header(ostream);
            fclose(ostream);
        }

        {
            FILE *istream = util_fopen("file_kw", "r");
            auto disk_kw = FileKW::read(istream, 1);
            test_assert_true(file_kw0 == *disk_kw[0]);

            /* Beyond the end of stream - return NULL */
            test_assert_throw(FileKW::read(istream,1), std::exception);
            fclose(istream);
        }

        {
            FILE *ostream = util_fopen("file_kw", "w");
            file_kw0.write_header(ostream);
            file_kw1.write_header(ostream);
            file_kw2.write_header(ostream);
            fclose(ostream);
        }
        {
            FILE *istream = util_fopen("file_kw", "r");
            auto disk_kw = FileKW::read(istream, 3);
            test_assert_true(file_kw0 == *disk_kw[0]);
            test_assert_true(file_kw1 == *disk_kw[1]);
            test_assert_true(file_kw2 == *disk_kw[2]);
            fclose(istream);
        }
        {
            FILE *istream = util_fopen("file_kw", "r");
            test_assert_throw(FileKW::read(istream, 10), std::exception);
            fclose(istream);
        }
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_file_kw_equal();
    test_create_file_kw();
}
