#include <cstdlib>
#include <unistd.h>

#include <string>
#include <memory>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_flag.hpp>
#include <resdata/rd_kw.hpp>

void test_writable(const char *src_file) {
    rd::util::TestArea ta("file_writable");
    char *fname = util_split_alloc_filename(src_file);

    ta.copy_file(src_file);
    {
        auto rd_file = rd::File::open(fname, FileMode::WRITABLE);
        rd::KW *swat = rd_file->get_kw("SWAT", 0);
        rd::KW swat0{*swat};
        test_assert_true(*swat == swat0);
        swat->at<float>(0) = 1000.0;
        rd_file->save_kw(swat);
        test_assert_true(rd_file->is_writable());

        auto rd_file2 = rd::File::open(fname);
        swat = rd_file2->get_kw("SWAT", 0);
        test_assert_true(util_double_approx_equal(swat->at<float>(0), 1000));
    }
}

int main(int argc, char **argv) {
    const char *src_file = argv[1];
    {
        rd::util::TestArea ta("file_equinor");

        ta.copy_file(src_file);

        test_writable(src_file);
    }
    exit(0);
}
