#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

int main(int argc, char **argv) {
    rd_kw_ptr rd_kw1 = make_rd_kw("KW", 10, RD_INT);
    int data[10];
    for (int i = 0; i < 10; i++) {
        rd_kw_iset_int(rd_kw1.get(), i, i);
        data[i] = i;
    }

    {
        auto rd_kw2 = std::make_unique<rd_kw_struct>(*rd_kw1.get());

        test_assert_true(rd_kw_equal(rd_kw1.get(), rd_kw2.get()));

        rd_kw_iset_int(rd_kw2.get(), 1, 77);
        test_assert_false(rd_kw_equal(rd_kw1.get(), rd_kw2.get()));
        rd_kw_iset_int(rd_kw2.get(), 1, 1);
        test_assert_true(rd_kw_equal(rd_kw1.get(), rd_kw2.get()));

        rd_kw_set_header_name(rd_kw2.get(), "TEST");
        test_assert_false(rd_kw_equal(rd_kw1.get(), rd_kw2.get()));
        test_assert_true(rd_kw_content_equal(rd_kw1.get(), rd_kw2.get()));
    }

    {
        rd_kw_type rd_ikw{"KW", 10, RD_INT, rd_kw_struct::shared_ref{data}};
        rd_kw_type rd_fkw{"KW", 10, RD_FLOAT, rd_kw_struct::shared_ref{data}};

        test_assert_true(rd_kw_content_equal(rd_kw1.get(), &rd_ikw));
        test_assert_false(rd_kw_content_equal(rd_kw1.get(), &rd_fkw));
    }

    test_assert_true(rd_kw_data_equal(rd_kw1.get(), data));
    data[0] = 99;
    test_assert_false(rd_kw_data_equal(rd_kw1.get(), data));
}
