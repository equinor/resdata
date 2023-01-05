#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>

int main(int argc, char **argv) {
    time_t t1 = util_make_date_utc(1, 1, 2000);
    time_t t2 = util_make_date_utc(1, 1, 2001);

    test_assert_true(util_before(t1, t2));
    test_assert_true(util_after(t2, t1));

    test_assert_false(util_before(t2, t1));
    test_assert_false(util_after(t1, t2));

    test_assert_false(util_before(t1, t1));
    test_assert_false(util_after(t1, t1));

    exit(0);
}
