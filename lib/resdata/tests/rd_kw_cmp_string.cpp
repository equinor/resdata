#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

void test_cmp_string() {
    rd_kw_type *rd_kw = rd_kw_alloc("HEADER", 1, RD_CHAR);

    rd_kw_iset_string8(rd_kw, 0, "ABCD");

    test_assert_int_equal(0, strcmp(rd_kw_iget_char_ptr(rd_kw, 0), "ABCD    "));
    test_assert_true(rd_kw_icmp_string(rd_kw, 0, "ABCD"));
    test_assert_true(rd_kw_icmp_string(rd_kw, 0, "ABCD    "));
    test_assert_true(rd_kw_icmp_string(rd_kw, 0, "ABCD "));

    test_assert_false(rd_kw_icmp_string(rd_kw, 0, "Different"));
    test_assert_false(rd_kw_icmp_string(rd_kw, 0, ""));
    test_assert_false(rd_kw_icmp_string(rd_kw, 0, ""));

    rd_kw_free(rd_kw);
}

int main(int argc, char **argv) {
    test_cmp_string();

    exit(0);
}
