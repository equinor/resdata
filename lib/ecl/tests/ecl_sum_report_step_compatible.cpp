#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_sum.hpp>

int main(int argc, char **argv) {
    const char *case1 = argv[1];
    const char *case2 = argv[2];
    const char *compatible_string = argv[3];
    ecl_sum_type *ecl_sum1 = ecl_sum_fread_alloc_case(case1, ":");
    ecl_sum_type *ecl_sum2 = ecl_sum_fread_alloc_case(case2, ":");

    test_assert_true(ecl_sum_is_instance(ecl_sum1));
    test_assert_true(ecl_sum_is_instance(ecl_sum2));
    test_assert_true(ecl_sum_report_step_compatible(ecl_sum1, ecl_sum1));
    test_assert_true(ecl_sum_report_step_compatible(ecl_sum2, ecl_sum2));

    {
        bool compatible;
        test_assert_true(util_sscanf_bool(compatible_string, &compatible));
        test_assert_bool_equal(
            compatible, ecl_sum_report_step_compatible(ecl_sum1, ecl_sum2));
    }
    ecl_sum_free(ecl_sum1);
    ecl_sum_free(ecl_sum2);
    exit(0);
}
