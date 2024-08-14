
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_endian_flip.hpp>

void test_close_stream2(const char *src_file, const char *target_file) {
    util_copy_file(src_file, target_file);
    rd_file_type *rd_file = rd_file_open(target_file, RD_FILE_CLOSE_STREAM);

    rd_file_load_all(rd_file);
    unlink(target_file);
    rd_kw_type *kw2 = rd_file_iget_kw(rd_file, 2);
    test_assert_not_NULL(kw2);
    rd_file_close(rd_file);
}

void test_loadall(const char *src_file, const char *target_file) {
    util_copy_file(src_file, target_file);
    {
        rd_file_type *rd_file = rd_file_open(target_file, RD_FILE_CLOSE_STREAM);

        test_assert_true(rd_file_load_all(rd_file));
        rd_file_close(rd_file);
    }

    {
        rd_file_type *rd_file = rd_file_open(target_file, RD_FILE_CLOSE_STREAM);
        unlink(target_file);

        test_assert_false(rd_file_load_all(rd_file));
        rd_file_close(rd_file);
    }
}

void test_close_stream1(const char *src_file, const char *target_file) {
    util_copy_file(src_file, target_file);

    rd_file_type *rd_file = rd_file_open(target_file, RD_FILE_CLOSE_STREAM);
    rd_kw_type *kw0 = rd_file_iget_kw(rd_file, 0);
    rd_kw_type *kw1 = rd_file_iget_kw(rd_file, 1);
    unlink(target_file);
    rd_kw_type *kw1b = rd_file_iget_kw(rd_file, 1);

    test_assert_not_NULL(kw0);
    test_assert_not_NULL(kw1);
    test_assert_ptr_equal(kw1, kw1b);

    rd_kw_type *kw2 = rd_file_iget_kw(rd_file, 2);
    test_assert_NULL(kw2);

    test_assert_false(rd_file_writable(rd_file));

    rd_file_close(rd_file);
}

void test_writable(const char *src_file) {
    rd::util::TestArea ta("file_writable");
    char *fname = util_split_alloc_filename(src_file);

    ta.copy_file(src_file);
    {
        rd_file_type *rd_file = rd_file_open(fname, RD_FILE_WRITABLE);
        rd_kw_type *swat = rd_file_iget_named_kw(rd_file, "SWAT", 0);
        rd_kw_type *swat0 = rd_kw_alloc_copy(swat);
        test_assert_true(rd_kw_equal(swat, swat0));
        rd_kw_iset_float(swat, 0, 1000.0);
        rd_file_save_kw(rd_file, swat);
        test_assert_true(rd_file_writable(rd_file));
        rd_file_close(rd_file);

        rd_file = rd_file_open(fname, 0);
        swat = rd_file_iget_named_kw(rd_file, "SWAT", 0);
        test_assert_true(
            util_double_approx_equal(rd_kw_iget_float(swat, 0), 1000));
    }
}

int main(int argc, char **argv) {
    const char *src_file = argv[1];
    const char *target_file = argv[2];

    {
        rd::util::TestArea ta("file_equinor");

        ta.copy_file(src_file);
        test_loadall(src_file, target_file);

        test_close_stream1(src_file, target_file);
        test_close_stream2(src_file, target_file);
        test_writable(src_file);
    }
    exit(0);
}
