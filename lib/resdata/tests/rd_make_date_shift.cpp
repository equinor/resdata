#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>

void test_date(int mday, int month, int year, int *year_offset) {
    time_t t0 = rd_make_date__(mday, month, year, year_offset);
    time_t t1 = util_make_date_utc(mday, month, year + *year_offset);
    test_assert_time_t_equal(t0, t1);
}

void test_offset(int mday, int month, int year, int current_offset) {
    int year_offset;
    time_t t0 = rd_make_date__(mday, month, year, &year_offset);
    time_t t1 = util_make_date_utc(mday, month, year + current_offset);

    test_assert_time_t_equal(t0, t1);
    test_assert_int_equal(current_offset, year_offset);
}

int main(int argc, char **argv) {
    int year_offset;
    test_date(1, 1, 0, &year_offset);
    test_offset(1, 1, 1000, year_offset);
    exit(0);
}
