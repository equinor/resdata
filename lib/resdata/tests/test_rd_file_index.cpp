#include <cstdio>
#include <utime.h>

#include <ios>
#include <memory>
#include <string>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

void test_load_nonexisting_file() {
    test_assert_throw(
        rd::File::fast_open("base_file", "a_file_that_does_not_exist_2384623"),
        std::ios_base::failure);
}

void test_create_and_load_index_file() {

    rd::util::TestArea ta("Load_index");
    {
        const char *file_name = "initial_data_file";
        const std::string index_file_name = "index_file";

        //creating the data file
        int data_size = 10;
        rd_kw_type *kw1 = rd_kw_alloc("TEST1_KW", data_size, RD_INT);
        for (int i = 0; i < data_size; ++i)
            rd_kw_iset_int(kw1, i, 537 + i);
        ERT::FortIO fortio(file_name, std::ios_base::out);
        rd_kw_fwrite(kw1, fortio);

        data_size = 5;
        rd_kw_type *kw2 = rd_kw_alloc("TEST2_KW", data_size, RD_FLOAT);
        for (int i = 0; i < data_size; ++i)
            rd_kw_iset_float(kw2, i, 0.15 * i);
        rd_kw_fwrite(kw2, fortio);
        fortio.fflush();
        //finished creating data file

        //creating rd_file
        auto rd_file = rd::File::open(file_name);
        test_assert_true(rd_file->has_kw("TEST1_KW"));
        rd_file->write_index(index_file_name);
        int rd_file_size = rd_file_get_size(rd_file.get());
        //finished using rd_file

        test_assert_throw(rd::File::fast_open(file_name, "nofile"),
                          std::ios_base::failure);
        test_assert_throw(rd::File::fast_open("nofile", index_file_name),
                          std::ios_base::failure);

        struct utimbuf tm1 = {1, 1};
        struct utimbuf tm2 = {2, 2};
        utime(file_name, &tm2);
        utime(index_file_name.c_str(), &tm1);
        utime(file_name, &tm1);
        utime(index_file_name.c_str(), &tm2);

        rd_file_ptr rd_file_index =
            rd::File::fast_open(file_name, index_file_name);

        test_assert_int_equal(rd_file_size,
                              rd_file_get_size(rd_file_index.get()));

        test_assert_true(rd_file_index->has_kw("TEST1_KW"));
        test_assert_true(rd_file_index->has_kw("TEST2_KW"));

        rd_kw_type *kwi1 = rd_file_iget_kw(rd_file_index.get(), 0);
        test_assert_true(rd_kw_equal(kw1, kwi1));
        test_assert_double_equal(537.0, rd_kw_iget_as_double(kwi1, 0));
        test_assert_double_equal(546.0, rd_kw_iget_as_double(kwi1, 9));

        rd_kw_type *kwi2 = rd_file_iget_kw(rd_file_index.get(), 1);
        test_assert_true(rd_kw_equal(kw2, kwi2));
        test_assert_double_equal(0.15, rd_kw_iget_as_double(kwi2, 1));
        test_assert_double_equal(0.60, rd_kw_iget_as_double(kwi2, 4));

        rd_kw_free(kw1);
        rd_kw_free(kw2);
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_load_nonexisting_file();
    test_create_and_load_index_file();
}
