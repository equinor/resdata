#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/vector.hpp>
#include <ert/util/util.h>
#include <ert/util/test_util.hpp>

void test_util_strstr() {
    test_assert_NULL(util_strstr_int_format("StringWithoutIntFormatSpecifier"));
    {
        const char *source = "String with %d ---";
        char *next = util_strstr_int_format(source);
        test_assert_ptr_equal(next, &source[13]);
    }

    {
        const char *source = "String with %04d ---";
        char *next = util_strstr_int_format(source);
        test_assert_ptr_equal(next, &source[15]);
    }

    {
        const char *source = "String with %ld ---";
        test_assert_NULL(util_strstr_int_format(source));
    }

    {
        const char *source = "String with %3d ---";
        test_assert_NULL(util_strstr_int_format(source));
    }
}

void test_util_count_int_format() {
    test_assert_int_equal(0, util_int_format_count("Abcdddd"));
    test_assert_int_equal(0, util_int_format_count("%4d"));
    test_assert_int_equal(0, util_int_format_count("%ld"));
    test_assert_int_equal(1, util_int_format_count("%04d"));
    test_assert_int_equal(2, util_int_format_count("%04dXX%034d"));
    test_assert_int_equal(4, util_int_format_count("%04dXX%034d%d%d"));
}

int main(int argc, char **argv) {
    test_util_strstr();
    test_util_count_int_format();
    exit(0);
}
