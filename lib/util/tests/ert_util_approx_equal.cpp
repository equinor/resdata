#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>

int main(int argc, char **argv) {

    test_assert_double_not_equal(-1.0, 1.0);
    test_assert_double_not_equal(0.00000000002, 0.000000000001);
    test_assert_double_not_equal(0.00000000002, 0.000000000001);

    test_assert_double_equal(1.00000000002, 1.000000000001);
    test_assert_double_equal(0.0, 0.0);
    test_assert_double_equal(0.75, asin(sin(0.75)));
    test_assert_double_equal(2.25, exp(log(2.25)));
    test_assert_double_equal(2.25, log(exp(2.25)));

    exit(0);
}
