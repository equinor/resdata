#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/ecl/ecl_util.hpp>

int main(int argc, char **argv) {
    const char *filename1 = argv[1];
    const char *filename2 = argv[2];
    const char *filename3 = argv[3];
    const char *filename4 = argv[4];
    const char *filename5 = argv[5];

    test_assert_int_equal(ecl_util_get_num_cpu(filename1), 4);
    test_assert_int_equal(ecl_util_get_num_cpu(filename2), 4);
    test_assert_int_equal(ecl_util_get_num_cpu(filename3), 15);
    test_assert_int_equal(ecl_util_get_num_cpu(filename4), 4);
    test_assert_int_equal(ecl_util_get_num_cpu(filename5), 4);
    exit(0);
}
