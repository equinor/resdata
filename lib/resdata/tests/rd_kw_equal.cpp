#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

int main(int argc, char **argv) {
    rd_kw_type *rd_kw1 = rd_kw_alloc("KW", 10, RD_INT);
    int data[10];
    int i;
    for (i = 0; i < 10; i++) {
        rd_kw_iset_int(rd_kw1, i, i);
        data[i] = i;
    }

    {
        rd_kw_type *rd_kw2 = rd_kw_alloc_copy(rd_kw1);

        test_assert_true(rd_kw_equal(rd_kw1, rd_kw2));

        rd_kw_iset_int(rd_kw2, 1, 77);
        test_assert_false(rd_kw_equal(rd_kw1, rd_kw2));
        rd_kw_iset_int(rd_kw2, 1, 1);
        test_assert_true(rd_kw_equal(rd_kw1, rd_kw2));

        rd_kw_set_header_name(rd_kw2, "TEST");
        test_assert_false(rd_kw_equal(rd_kw1, rd_kw2));
        test_assert_true(rd_kw_content_equal(rd_kw1, rd_kw2));
        rd_kw_free(rd_kw2);
    }

    {
        rd_kw_type *rd_ikw = rd_kw_alloc_new_shared("KW", 10, RD_INT, data);
        rd_kw_type *rd_fkw = rd_kw_alloc_new_shared("KW", 10, RD_FLOAT, data);

        test_assert_true(rd_kw_content_equal(rd_kw1, rd_ikw));
        test_assert_false(rd_kw_content_equal(rd_kw1, rd_fkw));

        rd_kw_free(rd_ikw);
        rd_kw_free(rd_fkw);
    }

    test_assert_true(rd_kw_data_equal(rd_kw1, data));
    data[0] = 99;
    test_assert_false(rd_kw_data_equal(rd_kw1, data));
    rd_kw_free(rd_kw1);
}
