#include <stdlib.h>

#include <ert/util/test_util.hpp>
#include <ert/util/statistics.hpp>

void test_mean_std() {
    double_vector_type *d = double_vector_alloc(0, 0);

    double_vector_append(d, 0);
    double_vector_append(d, 1);

    test_assert_double_equal(statistics_mean(d), 0.50);
    test_assert_double_equal(statistics_std(d), 0.50);

    double_vector_free(d);
}

int main(int argc, char **argv) { test_mean_std(); }
