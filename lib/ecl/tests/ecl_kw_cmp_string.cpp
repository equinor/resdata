#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_type.hpp>

void test_cmp_string() {
    ecl_kw_type *ecl_kw = ecl_kw_alloc("HEADER", 1, ECL_CHAR);

    ecl_kw_iset_string8(ecl_kw, 0, "ABCD");

    test_assert_int_equal(0,
                          strcmp(ecl_kw_iget_char_ptr(ecl_kw, 0), "ABCD    "));
    test_assert_true(ecl_kw_icmp_string(ecl_kw, 0, "ABCD"));
    test_assert_true(ecl_kw_icmp_string(ecl_kw, 0, "ABCD    "));
    test_assert_true(ecl_kw_icmp_string(ecl_kw, 0, "ABCD "));

    test_assert_false(ecl_kw_icmp_string(ecl_kw, 0, "Different"));
    test_assert_false(ecl_kw_icmp_string(ecl_kw, 0, ""));
    test_assert_false(ecl_kw_icmp_string(ecl_kw, 0, ""));

    ecl_kw_free(ecl_kw);
}

int main(int argc, char **argv) {
    test_cmp_string();

    exit(0);
}
