#include <cstdlib>

#include <ios>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>

void test_truncated(const char *filename, offset_type truncate_size) {
    {
        FILE *stream = util_fopen(filename, "r+");
        util_ftruncate(stream, truncate_size);
        fclose(stream);
    }
    {
        ERT::FortIO fortio(filename, std::ios_base::in, false, true);
        auto kw2 = rd_kw_struct::fread(fortio);
        test_assert_NULL(kw2.get());
    }
}

void test_fread_alloc() {
    rd::util::TestArea ta("fread_alloc");
    {
        auto kw1 = make_rd_kw("INT", 100, RD_INT);
        int i;
        for (i = 0; i < 100; i++)
            rd_kw_iset_int(kw1.get(), i, i);
        {
            ERT::FortIO fortio("INT", std::ios_base::out, false, true);
            rd_kw_fwrite(kw1.get(), fortio);
        }
        {
            ERT::FortIO fortio("INT", std::ios_base::in, false, true);
            rd_kw_ptr kw2 = rd_kw_struct::fread(fortio);
            test_assert_true(rd_kw_equal(kw1.get(), kw2.get()));
        }

        {
            offset_type file_size = util_file_size("INT");
            test_truncated("INT", file_size - 4);
            test_truncated("INT", file_size - 25);
            test_truncated("INT", 5);
            test_truncated("INT", 0);
        }
    }
}

void test_kw_io_charlength() {
    rd::util::TestArea ta("io_charlength");
    {
        const char *KW0 = "QWERTYUI";
        const char *KW1 = "ABCDEFGHIJTTTTTTTTTTTTTTTTTTTTTTABCDEFGHIJKLMNOP";
        rd_kw_ptr rd_kw_out0 = make_rd_kw(KW0, 5, RD_FLOAT);
        rd_kw_ptr rd_kw_out1 = make_rd_kw(KW1, 5, RD_FLOAT);
        for (int i = 0; i < rd_kw_get_size(rd_kw_out1.get()); i++) {
            rd_kw_iset_float(rd_kw_out0.get(), i, i * 1.5);
            rd_kw_iset_float(rd_kw_out1.get(), i, i * 1.5);
        }

        {
            ERT::FortIO f("TEST1", std::ios_base::out);
            test_assert_true(rd_kw_fwrite(rd_kw_out0.get(), f));
            test_assert_false(rd_kw_fwrite(rd_kw_out1.get(), f));
        }

        {
            test_assert_false(util_file_exists("TEST1"));
        }
    }
}

int main(int argc, char **argv) {
    test_fread_alloc();
    test_kw_io_charlength();
    exit(0);
}
