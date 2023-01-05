#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_type.hpp>

int main(int argc, char **argv) {
    ecl_kw_type *ecl_kw1 = ecl_kw_alloc("KW", 10, ECL_INT);
    int data[10];
    int i;
    for (i = 0; i < 10; i++) {
        ecl_kw_iset_int(ecl_kw1, i, i);
        data[i] = i;
    }

    {
        ecl_kw_type *ecl_kw2 = ecl_kw_alloc_copy(ecl_kw1);

        test_assert_true(ecl_kw_equal(ecl_kw1, ecl_kw2));

        ecl_kw_iset_int(ecl_kw2, 1, 77);
        test_assert_false(ecl_kw_equal(ecl_kw1, ecl_kw2));
        ecl_kw_iset_int(ecl_kw2, 1, 1);
        test_assert_true(ecl_kw_equal(ecl_kw1, ecl_kw2));

        ecl_kw_set_header_name(ecl_kw2, "TEST");
        test_assert_false(ecl_kw_equal(ecl_kw1, ecl_kw2));
        test_assert_true(ecl_kw_content_equal(ecl_kw1, ecl_kw2));
        ecl_kw_free(ecl_kw2);
    }

    {
        ecl_kw_type *ecl_ikw = ecl_kw_alloc_new_shared("KW", 10, ECL_INT, data);
        ecl_kw_type *ecl_fkw =
            ecl_kw_alloc_new_shared("KW", 10, ECL_FLOAT, data);

        test_assert_true(ecl_kw_content_equal(ecl_kw1, ecl_ikw));
        test_assert_false(ecl_kw_content_equal(ecl_kw1, ecl_fkw));

        ecl_kw_free(ecl_ikw);
        ecl_kw_free(ecl_fkw);
    }

    test_assert_true(ecl_kw_data_equal(ecl_kw1, data));
    data[0] = 99;
    test_assert_false(ecl_kw_data_equal(ecl_kw1, data));
    ecl_kw_free(ecl_kw1);
}
