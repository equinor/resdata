#include <ert/util/test_util.hpp>
#include <ert/ecl/ecl_util.hpp>

int main(int argc, char **argv) {
    test_assert_true(ecl_util_valid_basename_fmt("ecl_%d.data"));
    test_assert_true(ecl_util_valid_basename_fmt("ECL_%d.DATA"));
    test_assert_true(ecl_util_valid_basename_fmt("ECL_%04d.DATA"));
    test_assert_true(ecl_util_valid_basename_fmt("mypath/ECL_%04d.DATA"));
    test_assert_true(ecl_util_valid_basename_fmt("MYPATH/ECL_%04d.DATA"));
    test_assert_true(ecl_util_valid_basename_fmt("MYPATH/ECL_%04d.DATA"));
    test_assert_true(ecl_util_valid_basename_fmt("ECL_%d.dATA"));
    test_assert_false(ecl_util_valid_basename_fmt("ECL_%s.DATA"));
    test_assert_false(ecl_util_valid_basename_fmt("ECL_%f.DATA"));
    test_assert_true(ecl_util_valid_basename_fmt("mypath/ECL_%d.dATA"));

    exit(0);
}
