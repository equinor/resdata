#include <cstdlib>

#include <ios>
#include <utility>
#include <vector>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>

void test_fread_alloc() {
    rd::util::TestArea ta("fread_alloc");
    {
        std::vector<int> data(100);
        for (size_t i = 0; i < 100; i++)
            data[i] = i;
        rd::KW kw1{"INT", std::move(data)};
        {
            ERT::FortIO fortio("INT", std::ios_base::out, false, true);
            kw1.fwrite(fortio);
        }
        {
            ERT::FortIO fortio("INT", std::ios_base::in, false, true);
            auto kw2 = rd::KW::fread(fortio);
            test_assert_true(kw1 == *kw2.get());
        }
    }
}

void test_kw_io_charlength() {
    rd::util::TestArea ta("io_charlength");
    {
        const char *KW0 = "QWERTYUI";
        const char *KW1 = "ABCDEFGHIJTTTTTTTTTTTTTTTTTTTTTTABCDEFGHIJKLMNOP";
        std::vector<float> data(5);
        for (size_t i = 0; i < 5; i++)
            data[i] = i * 1.5f;
        rd::KW rd_kw_out0{KW0, data};
        rd::KW rd_kw_out1{KW1, std::move(data)};

        {
            ERT::FortIO f("TEST1", std::ios_base::out);
            test_assert_true(rd_kw_out0.fwrite(f));
            test_assert_false(rd_kw_out1.fwrite(f));
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
