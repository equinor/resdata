#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_sum.hpp>

void test_time_range(const ecl_sum_type *ecl_sum) {
    // Hardcoded Gurbat case values
    time_t start = ecl_util_make_date(1, 1, 2000);
    time_t end = ecl_util_make_date(31, 12, 2004);

    test_assert_time_t_equal(ecl_sum_get_start_time(ecl_sum), start);
    test_assert_time_t_equal(ecl_sum_get_end_time(ecl_sum), end);
    test_assert_time_t_equal(ecl_sum_get_data_start(ecl_sum), start);
}

void test_days(const ecl_sum_type *ecl_sum) {
    time_t date1 = ecl_util_make_date(1, 1, 2000);
    time_t date2 = ecl_util_make_date(31, 12, 2004);
    time_t date3 = ecl_util_make_date(2, 1, 2000);

    double days1 = ecl_sum_time2days(ecl_sum, date1);
    double days2 = ecl_sum_time2days(ecl_sum, date2);
    double days3 = ecl_sum_time2days(ecl_sum, date3);

    test_assert_double_equal(days1, 0);
    test_assert_double_equal(days2, 1826);
    test_assert_double_equal(days3, 1);
}

void test_is_oil_producer(const ecl_sum_type *ecl_sum) {
    test_assert_true(ecl_sum_is_oil_producer(ecl_sum, "OP_1"));
    test_assert_false(ecl_sum_is_oil_producer(ecl_sum, "WI_1"));
    test_assert_false(ecl_sum_is_oil_producer(ecl_sum, "DoesNotExist"));
}

int main(int argc, char **argv) {
    const char *case1 = argv[1];

    ecl_sum_type *ecl_sum1 = ecl_sum_fread_alloc_case(case1, ":");

    test_assert_true(ecl_sum_is_instance(ecl_sum1));
    test_time_range(ecl_sum1);
    test_days(ecl_sum1);
    test_is_oil_producer(ecl_sum1);
    ecl_sum_free(ecl_sum1);
    exit(0);
}
