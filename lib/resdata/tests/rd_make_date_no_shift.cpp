#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>

void test_date(int mday, int month, int year) {
    time_t t0 = rd_make_date(mday, month, year);
    time_t t1 = util_make_date_utc(mday, month, year);

    test_assert_time_t_equal(t0, t1);
}

void test_offset(int mday, int month, int year) {
    int year_offset;
    rd_make_date__(mday, month, year, &year_offset);
    test_assert_int_equal(0, year_offset);
}

int main(int argc, char **argv) {
    test_date(10, 10, 2000);
    test_offset(10, 10, 2000);
    // test_assert_util_abort( make_date( 1 , 1 , 0);
    exit(0);
}
