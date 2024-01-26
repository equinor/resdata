#include <stdio.h>
#include <utime.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>

void test_load_nonexisting_file() {
    rd_file_type *rd_file =
        rd_file_fast_open("base_file", "a_file_that_does_not_exist_2384623", 0);
    test_assert_NULL(rd_file);
}

void test_create_and_load_index_file() {

    rd::util::TestArea ta("Load_index");
    {
        const char *file_name = "initial_data_file";
        const char *index_file_name = "index_file";

        //creating the data file
        int data_size = 10;
        rd_kw_type *kw1 = rd_kw_alloc("TEST1_KW", data_size, RD_INT);
        for (int i = 0; i < data_size; ++i)
            rd_kw_iset_int(kw1, i, 537 + i);
        fortio_type *fortio =
            fortio_open_writer(file_name, false, RD_ENDIAN_FLIP);
        rd_kw_fwrite(kw1, fortio);

        data_size = 5;
        rd_kw_type *kw2 = rd_kw_alloc("TEST2_KW", data_size, RD_FLOAT);
        for (int i = 0; i < data_size; ++i)
            rd_kw_iset_float(kw2, i, 0.15 * i);
        rd_kw_fwrite(kw2, fortio);
        fortio_fclose(fortio);
        //finished creating data file

        //creating rd_file
        rd_file_type *rd_file = rd_file_open(file_name, 0);
        test_assert_true(rd_file_has_kw(rd_file, "TEST1_KW"));
        rd_file_write_index(rd_file, index_file_name);
        int rd_file_size = rd_file_get_size(rd_file);
        rd_file_close(rd_file);
        //finished using rd_file

        test_assert_false(rd_file_index_valid(file_name, "nofile"));
        test_assert_false(rd_file_index_valid("nofile", index_file_name));

        struct utimbuf tm1 = {1, 1};
        struct utimbuf tm2 = {2, 2};
        utime(file_name, &tm2);
        utime(index_file_name, &tm1);
        test_assert_false(rd_file_index_valid(file_name, index_file_name));
        utime(file_name, &tm1);
        utime(index_file_name, &tm2);
        test_assert_true(rd_file_index_valid(file_name, index_file_name));

        rd_file_type *rd_file_index =
            rd_file_fast_open(file_name, index_file_name, 0);
        test_assert_true(rd_file_is_instance(rd_file_index));
        test_assert_true(rd_file_get_global_view(rd_file_index));

        test_assert_int_equal(rd_file_size, rd_file_get_size(rd_file_index));

        test_assert_true(rd_file_has_kw(rd_file_index, "TEST1_KW"));
        test_assert_true(rd_file_has_kw(rd_file_index, "TEST2_KW"));

        rd_kw_type *kwi1 = rd_file_iget_kw(rd_file_index, 0);
        test_assert_true(rd_kw_equal(kw1, kwi1));
        test_assert_double_equal(537.0, rd_kw_iget_as_double(kwi1, 0));
        test_assert_double_equal(546.0, rd_kw_iget_as_double(kwi1, 9));

        rd_kw_type *kwi2 = rd_file_iget_kw(rd_file_index, 1);
        test_assert_true(rd_kw_equal(kw2, kwi2));
        test_assert_double_equal(0.15, rd_kw_iget_as_double(kwi2, 1));
        test_assert_double_equal(0.60, rd_kw_iget_as_double(kwi2, 4));

        rd_kw_free(kw1);
        rd_kw_free(kw2);
        rd_file_close(rd_file_index);
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_load_nonexisting_file();
    test_create_and_load_index_file();
}
