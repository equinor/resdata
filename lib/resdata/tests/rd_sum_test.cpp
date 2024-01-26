#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_sum.hpp>

void test_time_range(const rd_sum_type *rd_sum) {
    // Hardcoded Gurbat case values
    time_t start = rd_make_date(1, 1, 2000);
    time_t end = rd_make_date(31, 12, 2004);

    test_assert_time_t_equal(rd_sum_get_start_time(rd_sum), start);
    test_assert_time_t_equal(rd_sum_get_end_time(rd_sum), end);
    test_assert_time_t_equal(rd_sum_get_data_start(rd_sum), start);
}

void test_days(const rd_sum_type *rd_sum) {
    time_t date1 = rd_make_date(1, 1, 2000);
    time_t date2 = rd_make_date(31, 12, 2004);
    time_t date3 = rd_make_date(2, 1, 2000);

    double days1 = rd_sum_time2days(rd_sum, date1);
    double days2 = rd_sum_time2days(rd_sum, date2);
    double days3 = rd_sum_time2days(rd_sum, date3);

    test_assert_double_equal(days1, 0);
    test_assert_double_equal(days2, 1826);
    test_assert_double_equal(days3, 1);
}

void test_is_oil_producer(const rd_sum_type *rd_sum) {
    test_assert_true(rd_sum_is_oil_producer(rd_sum, "OP_1"));
    test_assert_false(rd_sum_is_oil_producer(rd_sum, "WI_1"));
    test_assert_false(rd_sum_is_oil_producer(rd_sum, "DoesNotExist"));
}

int main(int argc, char **argv) {
    const char *case1 = argv[1];

    rd_sum_type *rd_sum1 = rd_sum_fread_alloc_case(case1, ":");

    test_assert_true(rd_sum_is_instance(rd_sum1));
    test_time_range(rd_sum1);
    test_days(rd_sum1);
    test_is_oil_producer(rd_sum1);
    rd_sum_free(rd_sum1);
    exit(0);
}
