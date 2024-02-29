#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

void test_sscanf_bool() {
    char const *expected_true = "1\0___"
                                "T\0___"
                                "True\0"
                                "tRuE";
    int const expected_true_step = 5;
    char const *const expected_true_end =
        expected_true + 4 * expected_true_step;

    char const *expected_false = "0\0____"
                                 "F\0____"
                                 "False\0"
                                 "fALse";
    int const expected_false_step = 6;
    char const *const expected_false_end =
        expected_false + 4 * expected_false_step;

    char const *expected_fail = "\0___"
                                "t\0___"
                                "f\0___"
                                "Tru\0"
                                "asd";
    int const expected_fail_step = 4;
    char const *const expected_fail_end =
        expected_fail + 5 * expected_fail_step;

    bool value = false;

    for (; expected_true < expected_true_end;
         expected_true += expected_true_step) {
        value = false;
        test_assert_true(util_sscanf_bool(expected_true, &value));
        test_assert_bool_equal(value, true);
    }

    for (; expected_false < expected_false_end;
         expected_false += expected_false_step) {
        value = true;
        test_assert_true(util_sscanf_bool(expected_false, &value));
        test_assert_bool_equal(value, false);
    }

    for (; expected_fail < expected_fail_end;
         expected_fail += expected_fail_step) {
        value = true;
        test_assert_false(util_sscanf_bool(expected_fail, &value));
        test_assert_bool_equal(value, false);
    }

    // Test null buffer
    value = true;
    test_assert_false(util_sscanf_bool(NULL, &value));
    test_assert_bool_equal(value, false);

    test_assert_false(util_sscanf_bool(NULL, NULL));
}

void test_sscanf_double() {
    double value = 1.0;
    test_assert_true(util_sscanf_double("0.0", &value));
    test_assert_double_equal(value, 0.0);

    test_assert_true(util_sscanf_double("47.35", &value));
    test_assert_double_equal(value, 47.35);

    test_assert_true(util_sscanf_double("-0.0", &value));
    test_assert_double_equal(value, 0.0);

    test_assert_true(util_sscanf_double("-54.1341", &value));
    test_assert_double_equal(value, -54.1341);

    test_assert_true(util_sscanf_double("-.1341", &value));
    test_assert_double_equal(value, -0.1341);

    test_assert_true(util_sscanf_double("+.284", &value));
    test_assert_double_equal(value, 0.284);

    test_assert_true(util_sscanf_double("-.45e-2", &value));
    test_assert_double_equal(value, -0.0045);

    test_assert_true(util_sscanf_double("0xFF", &value));
    test_assert_double_equal(value, 255);

    test_assert_true(util_sscanf_double("INF", &value));
    test_assert_double_equal(value, INFINITY);

    test_assert_true(util_sscanf_double("NaN", &value));
    test_assert_true(isnan(value));

    // double max and min
    char buffer[30];
    snprintf(buffer, 30, "-%.20g", DBL_MAX);
    test_assert_true(util_sscanf_double(buffer, &value));
    test_assert_double_equal(value, -DBL_MAX);

    snprintf(buffer, 30, "%.20g", DBL_MIN);
    test_assert_true(util_sscanf_double(buffer, &value));
    test_assert_double_equal(value, DBL_MIN);

    // Garbage characters
    value = 15.3;
    test_assert_false(util_sscanf_double("0x12GWS", &value));
    test_assert_double_equal(value, 15.3);

    test_assert_false(util_sscanf_double("--.+", &value));
    test_assert_double_equal(value, 15.3);

    // NULL buffer
    value = 15.3;
    test_assert_false(util_sscanf_double(NULL, &value));
    test_assert_double_equal(value, 15.3);

    test_assert_false(util_sscanf_double(NULL, NULL));
}

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

void check_iso_date(time_t expected, const char *date_string,
                    bool expected_return) {
    time_t t;
    bool valid = util_sscanf_isodate(date_string, &t);

    test_assert_bool_equal(valid, expected_return);
    if (valid)
        test_assert_time_t_equal(t, expected);
    else
        test_assert_time_t_equal(t, -1);
}

void test_sscanf_isodate() {
    time_t expected = util_make_date_utc(10, 11, 2011);
    check_iso_date(expected, "2011-11-10", true);

    /* Valid dates, but incorrectly formatted */
    test_assert_false(util_sscanf_isodate("2017.10.07", NULL));
    test_assert_false(util_sscanf_isodate("07.10.2017", NULL));
    test_assert_false(util_sscanf_isodate("7.10.2017", NULL));
    test_assert_false(util_sscanf_isodate("17.1.2017", NULL));
    test_assert_false(util_sscanf_isodate("17-01-2017", NULL));
    test_assert_false(util_sscanf_isodate("2017-10.7", NULL));
    test_assert_false(util_sscanf_isodate("2017/10/07", NULL));
    test_assert_false(util_sscanf_isodate("07/10/2017", NULL));

    test_assert_false(util_sscanf_isodate("217-07-10", NULL)); // year 217

    /* ISO8601 does not support year 10000 */
    test_assert_false(util_sscanf_isodate("10000-01-01", NULL));

    /* Invalid dates, correctly formatted */
    test_assert_false(util_sscanf_isodate("2017-15-07", NULL));
    test_assert_false(util_sscanf_isodate("2017-10-47", NULL));

    // Test NULL buffer
    check_iso_date(expected, NULL, false);
    test_assert_false(util_sscanf_isodate(NULL, NULL));
}

int main(int argc, char **argv) {
    test_sscanf_bool();
    test_sscanf_double();
    test_sscanf_int();
    test_sscanf_isodate();
    exit(0);
}
