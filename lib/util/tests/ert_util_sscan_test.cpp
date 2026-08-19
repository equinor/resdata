#include <float.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

void test_sscanf_int() {
    int value = 1;
    test_assert_true(util_sscanf_int("0", &value));
    test_assert_int_equal(value, 0);

    test_assert_true(util_sscanf_int("241", &value));
    test_assert_int_equal(value, 241);

    test_assert_true(util_sscanf_int("-0", &value));
    test_assert_int_equal(value, 0);

    test_assert_true(util_sscanf_int("-852", &value));
    test_assert_int_equal(value, -852);

    value = 1;
    test_assert_false(util_sscanf_int("+-+-+-", &value));
    test_assert_int_equal(value, 1);

    test_assert_false(util_sscanf_int("7.5", &value));
    test_assert_int_equal(value, 1);

    test_assert_false(util_sscanf_int("abc1", &value));
    test_assert_int_equal(value, 1);

    test_assert_false(util_sscanf_int("", &value));
    test_assert_int_equal(value, 1);

    // max and min
    char buffer[30];
    snprintf(buffer, 30, "-%d", INT_MAX);
    test_assert_true(util_sscanf_int(buffer, &value));
    test_assert_int_equal(value, -INT_MAX);

    snprintf(buffer, 30, "%d", INT_MIN);
    test_assert_true(util_sscanf_int(buffer, &value));
    test_assert_int_equal(value, INT_MIN);

    // NULL buffer
    value = 9;
    test_assert_false(util_sscanf_int(NULL, &value));
    test_assert_int_equal(value, 9);

    test_assert_false(util_sscanf_int(NULL, NULL));
}

int main(int argc, char **argv) {
    test_sscanf_int();
    exit(0);
}
