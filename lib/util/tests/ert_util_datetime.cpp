#include <stdlib.h>
#include <stdbool.h>
#include <execinfo.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

int main(int argc, char **argv) {

    /* Invalid seconds */
    test_assert_false(
        util_make_datetime_utc_validated(75, 0, 0, 1, 1, 2010, NULL));

    /* Invalid month */
    test_assert_false(
        util_make_datetime_utc_validated(15, 0, 0, 1, 16, 2010, NULL));

    /* Invalid mday */
    test_assert_false(
        util_make_datetime_utc_validated(15, 0, 0, -1, 1, 2010, NULL));

    {
        time_t t;
        int sec, min, hour, mday, month, year;
        test_assert_true(
            util_make_datetime_utc_validated(15, 3, 6, 5, 7, 2010, &t));

        util_set_datetime_values_utc(t, &sec, &min, &hour, &mday, &month,
                                     &year);
        test_assert_int_equal(sec, 15);
        test_assert_int_equal(min, 3);
        test_assert_int_equal(hour, 6);
        test_assert_int_equal(mday, 5);
        test_assert_int_equal(month, 7);
        test_assert_int_equal(year, 2010);
    }
}
