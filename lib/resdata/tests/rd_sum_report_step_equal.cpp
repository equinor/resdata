#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>

#include <resdata/rd_sum.hpp>

int main(int argc, char **argv) {
    const char *case1 = argv[1];
    const char *case2 = argv[2];
    const char *equal_string = argv[3];
    bool equal;
    rd_sum_type *rd_sum1 = rd_sum_fread_alloc_case(case1, ":");
    rd_sum_type *rd_sum2 = rd_sum_fread_alloc_case(case2, ":");

    test_assert_true(rd_sum_is_instance(rd_sum1));
    test_assert_true(rd_sum_is_instance(rd_sum2));
    test_assert_true(rd_sum_report_step_equal(rd_sum1, rd_sum1));
    test_assert_true(util_sscanf_bool(equal_string, &equal));

    test_assert_true(rd_sum_report_step_equal(rd_sum1, rd_sum1));
    test_assert_true(rd_sum_report_step_equal(rd_sum2, rd_sum2));
    test_assert_bool_equal(equal, rd_sum_report_step_equal(rd_sum1, rd_sum2));

    rd_sum_free(rd_sum1);
    rd_sum_free(rd_sum2);
    exit(0);
}
