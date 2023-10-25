#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <resdata/rd_sum.hpp>

void has_r2r_key(const rd_sum_type *rd_sum) {
    test_assert_true(rd_sum_has_key(rd_sum, "RGF:393217"));
    test_assert_true(rd_sum_has_key(rd_sum, "RGF:1-2"));
    test_assert_true(rd_sum_has_key(rd_sum, "ROF:524291"));
    test_assert_true(rd_sum_has_key(rd_sum, "RWF:393222"));
    test_assert_true(rd_sum_has_key(rd_sum, "RWF:6-2"));
    test_assert_true(rd_sum_has_key(rd_sum, "ROF:3-6"));
    test_assert_true(rd_sum_has_key(rd_sum, "RNLF:458753"));
    test_assert_true(rd_sum_has_key(rd_sum, "RNLF:1-4"));
}

void get_unit(const rd_sum_type *rd_sum) {
    test_assert_string_equal(rd_sum_get_unit(rd_sum, "RGF:1-2"), "GFLOW");
    test_assert_string_equal(rd_sum_get_unit(rd_sum, "ROF:3-6"), "OFLOW");
    test_assert_string_equal(rd_sum_get_unit(rd_sum, "RWF:6-2"), "WFLOW");
    test_assert_string_equal(rd_sum_get_unit(rd_sum, "RNLF:1-4"), "NLFLOW");
}

void get_var_params_index(const rd_sum_type *rd_sum) {
    test_assert_int_equal(
        rd_sum_get_general_var_params_index(rd_sum, "RGF:1-2"), 21917);
    test_assert_int_equal(
        rd_sum_get_general_var_params_index(rd_sum, "ROF:3-6"), 21918);
    test_assert_int_equal(
        rd_sum_get_general_var_params_index(rd_sum, "RWF:6-2"), 21919);
    test_assert_int_equal(
        rd_sum_get_general_var_params_index(rd_sum, "RNLF:1-4"), 21920);
}

int main(int argc, char **argv) {

    const char *headerfile = argv[1];

    rd_sum_type *rd_sum = rd_sum_fread_alloc_case(headerfile, ":");

    if (rd_sum) {
        has_r2r_key(rd_sum);
        get_unit(rd_sum);
        get_var_params_index(rd_sum);
    } else
        test_error_exit("rd_region2region_test: Test file not read correctly");

    if (rd_sum)
        rd_sum_free(rd_sum);

    exit(0);
}
