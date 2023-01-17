#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/vector.hpp>
#include <ert/util/util.h>
#include <ert/util/test_util.hpp>

int main(int argc, char **argv) {
    double value;

    value = 0;
    util_clamp_double(&value, -1, 1);
    test_assert_double_equal(value, 0);

    value = 0;
    util_clamp_double(&value, 1, 2);
    test_assert_double_equal(value, 1);

    value = 0;
    util_clamp_double(&value, 2, 1);
    test_assert_double_equal(value, 1);

    value = 0;
    util_clamp_double(&value, -2, 0);
    test_assert_double_equal(value, 0);

    value = 0;
    util_clamp_double(&value, -2, -1);
    test_assert_double_equal(value, -1);

    value = 0;
    util_clamp_double(&value, -1, -2);
    test_assert_double_equal(value, -1);
}
